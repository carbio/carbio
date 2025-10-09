#include "carbio/fingerprint.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include <QThread>

#include <algorithm>
#include <bit>
#include <iomanip>
#include <sstream>

#if defined(__unix) || defined(__unix__) || defined(unix)
#include <linux/serial.h>
#include <sys/ioctl.h>
#endif

namespace carbio
{
static std::string bytesToHex(const std::vector<uint8_t> &bytes)
{
  std::ostringstream oss;
  for (auto b : bytes)
  {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
  }
  return oss.str();
}

static bool readExact(QSerialPort *serial, qint64 size, int32_t timeoutMs, QByteArray &out)
{
  out.clear();
  if (!serial || !serial->isOpen() || size <= 0)
    return false;

  QElapsedTimer timer;
  timer.start();

  while (out.size() < size)
  {
    qint64 available = serial->bytesAvailable();
    if (available > 0)
    {
      qint64     toRead = qMin(available, size - out.size());
      QByteArray chunk  = serial->read(toRead);
      if (!chunk.isEmpty())
      {
        out.append(chunk);
        continue;
      }
    }

    int remainTimeout = timeoutMs - static_cast<int>(timer.elapsed());
    if (remainTimeout <= 0)
    {
      //qDebug() << "Read timeout: expected" << size << "bytes, got" << out.size();
      return false;
    }

    if (!serial->waitForReadyRead(qMax(50, remainTimeout)))
    {
      if (out.size() >= size)
        break;
      if (out.size() > 0)
      {
        //qDebug() << "Partial read:" << out.size() << "/" << size << "bytes";
      }
      return false;
    }
  }

  return out.size() == size;
}

std::vector<uint8_t> FingerprintSensor::Packet::serialize() const
{
  std::vector<uint8_t> buffer;
  buffer.reserve(PACKET_HEADER_SIZE + data.size() + 2);

  // Start code (2 bytes)
  buffer.push_back(static_cast<uint8_t>(startCode >> 8));
  buffer.push_back(static_cast<uint8_t>(startCode & 0xFF));

  // Address (4 bytes)
  buffer.insert(buffer.end(), address.begin(), address.end());

  // Packet type (1 byte)
  buffer.push_back(static_cast<uint8_t>(id));

  // Length = data length + checksum (2 bytes)
  uint16_t const totalLength = static_cast<uint16_t>(data.size() + 2);
  buffer.push_back(static_cast<uint8_t>(totalLength >> 8));
  buffer.push_back(static_cast<uint8_t>(totalLength & 0xFF));

  // Data - N bytes
  buffer.insert(buffer.end(), data.begin(), data.end());

  // Calculate checksum
  uint16_t checksum = 0;
  checksum += static_cast<std::uint8_t>(id);
  checksum += static_cast<std::uint8_t>(totalLength >> 8);
  checksum += static_cast<std::uint8_t>(totalLength & 0xFF);

  for (auto byte : data)
  {
    checksum += byte;
  }

  // Checksum
  buffer.push_back(static_cast<uint8_t>(checksum >> 8));
  buffer.push_back(static_cast<uint8_t>(checksum & 0xFF));

  return buffer;
}

FingerprintSensor::FingerprintSensor(std::string_view devicePath)
    : m_serial(std::make_unique<QSerialPort>()),
      m_portName(devicePath),
      m_password(0x00000000),
      m_address(DEFAULT_ADDRESS)
{
}

FingerprintSensor::~FingerprintSensor()
{
  end();
}

FingerprintSensor::FingerprintSensor(FingerprintSensor &&other) noexcept
    : m_serial(std::move(other.m_serial)),
      m_portName(std::move(other.m_portName)),
      m_password(std::move(other.m_password)),
      m_address(other.m_address),
      m_sysParams(other.m_sysParams),
      m_lastSearchResult(other.m_lastSearchResult)
{
  other.m_address = DEFAULT_ADDRESS;
}

FingerprintSensor &FingerprintSensor::operator=(FingerprintSensor &&other) noexcept
{
  if (this != &other)
  {
    end();

    m_serial           = std::move(other.m_serial);
    m_portName         = std::move(other.m_portName);
    m_password         = std::move(other.m_password);
    m_address          = other.m_address;
    m_sysParams        = other.m_sysParams;
    m_lastSearchResult = other.m_lastSearchResult;

    other.m_address = DEFAULT_ADDRESS;
  }
  return *this;
}

bool FingerprintSensor::begin()
{
  const std::array<int32_t, 12> baudRates = {57600, 115200, 9600,  19200, 28800, 38400,
                                             48000, 67200,  76800, 86400, 96000, 105600};
  for (int32_t tryBaud : baudRates)
  {
    if (begin(tryBaud))
    {
      return true;
    }
  }
  return false;
}

bool FingerprintSensor::begin(int32_t baud)
{
  if (!m_serial)
  {
    return false;
  }

  m_serial->setPortName(m_portName.data());
  m_serial->setBaudRate(baud);
  m_serial->setReadBufferSize(512);
  m_serial->setFlowControl(QSerialPort::NoFlowControl);
  m_serial->setDataBits(QSerialPort::Data8);
  m_serial->setParity(QSerialPort::NoParity);
  m_serial->setStopBits(QSerialPort::OneStop);
  #if defined(__unix) || defined(__unix__) || defined(unix)
  struct serial_struct serial_struct {};
  if (ioctl(m_serial->handle(), TIOCGSERIAL, &serial_struct) == 0) {
    serial_struct.flags |= ASYNC_LOW_LATENCY;
    ioctl(m_serial->handle(), TIOCSSERIAL, &serial_struct);
  }
  #endif

  if (!m_serial->open(QIODevice::ReadWrite))
  {
    //qDebug() << "Failed to open serial port:" << m_serial->errorString();
    return false;
  }

  m_serial->clear();

  QThread::msleep(100); // Reduced from 1000ms for faster auto-detection

  qint64 available = m_serial->bytesAvailable();
  if (available > 0)
  {
    QByteArray startup = m_serial->readAll();
    //qDebug() << "Cleared" << available << "startup bytes from sensor:" << startup.toHex(' ');
  }

  if (verifyPassword() != StatusCode::Success)
  {
    //qDebug() << "Error verifying device password";
    end();
    return false;
  }

  if (!getParameters())
  {
    //qDebug() << "Error reading system parameters";
    end();
    return false;
  }

  qDebug() << "Serial port opened successfully at" << baud << "baud";
  return true;
}

void FingerprintSensor::end()
{
  if (m_serial && m_serial->isOpen())
  {
    m_serial->close();
  }
}

StatusCode FingerprintSensor::verifyPassword()
{
  std::array<std::uint8_t, 4> pwd{
      static_cast<std::uint8_t>((m_password >> 24) & 0xFFu), static_cast<std::uint8_t>((m_password >> 16) & 0xFFu),
      static_cast<std::uint8_t>((m_password >> 8) & 0xFFu), static_cast<std::uint8_t>(m_password & 0xFFu)};

  //qDebug() << "Verifying password:" << QString("0x%1").arg(m_password, 8, 16, QChar('0'));
  //qDebug() << "Password bytes:" << QString::fromStdString(bytesToHex({pwd.begin(), pwd.end()}));

  auto status = sendCommand(CommandCode::VerifyPassword, pwd);
  //qDebug() << "VerifyPassword status:" << QString("0x%1").arg(static_cast<int>(status), 2, 16, QChar('0'));

  return status;
}

std::optional<SystemParameters> FingerprintSensor::getParameters()
{
  auto result = sendCommandWithData(CommandCode::ReadSysParam, {});

  if (result.status != StatusCode::Success)
  {
    //qDebug() << "ReadSysParam failed with status:" << QString("0x%1").arg(static_cast<int>(result.status), 2, 16, QChar('0'));
    return std::nullopt;
  }

  // Response data format: [status, param_bytes...]
  // We need at least 17 bytes (1 status + 16 parameters)
  if (result.data.size() < 17)
  {
    //qDebug() << "Invalid response data size:" << result.data.size();
    return std::nullopt;
  }

  // Skip first byte (status), parse parameters starting at index 1
  auto const &d = result.data;

  m_sysParams.statusRegister = static_cast<uint16_t>((d[1] << 8) | d[2]);
  m_sysParams.systemId       = static_cast<uint16_t>((d[3] << 8) | d[4]);
  m_sysParams.capacity       = static_cast<uint16_t>((d[5] << 8) | d[6]);
  m_sysParams.SecurityLevel  = static_cast<uint16_t>((d[7] << 8) | d[8]);
  m_sysParams.deviceAddress  = static_cast<uint32_t>((d[9] << 24) | (d[10] << 16) | (d[11] << 8) | d[12]);
  m_sysParams.packetLength   = static_cast<uint16_t>((d[13] << 8) | d[14]);
  m_sysParams.baudRate       = static_cast<uint16_t>((d[15] << 8) | d[16]);

  return m_sysParams;
}

// Capture fingerprint image
StatusCode FingerprintSensor::captureImage()
{
  return sendCommand(CommandCode::GetImage);
}

// Convert image to template
StatusCode FingerprintSensor::imageToTemplate(uint8_t bufferSlot)
{
  std::array<uint8_t, 1> const data = {bufferSlot};
  return sendCommand(CommandCode::Image2Tz, data);
}

// Create model
StatusCode FingerprintSensor::createModel()
{
  return sendCommand(CommandCode::RegModel);
}

// Store template
StatusCode FingerprintSensor::storeTemplate(uint16_t templateId, uint8_t bufferSlot)
{
  std::array<uint8_t, 3> const data = {bufferSlot, // Buffer slot (default 1)
                                       static_cast<uint8_t>((templateId >> 8) & 0xFF),
                                       static_cast<uint8_t>(templateId & 0xFF)};
  return sendCommand(CommandCode::Store, data);
}

// Search for fingerprint
std::optional<SearchResult> FingerprintSensor::search(uint8_t bufferSlot, uint16_t startId, uint16_t count)
{
  if (count == 0)
  {
    count = m_sysParams.capacity;
  }

  std::array<uint8_t, 5> const data = {bufferSlot, static_cast<uint8_t>((startId >> 8) & 0xFF),
                                       static_cast<uint8_t>(startId & 0xFF), static_cast<uint8_t>((count >> 8) & 0xFF),
                                       static_cast<uint8_t>(count & 0xFF)};

  auto result = sendCommandWithData(CommandCode::Search, data);

  if (result.status != StatusCode::Success)
  {
    return std::nullopt;
  }

  // Response data format: [status, fingerID_hi, fingerID_lo, confidence_hi, confidence_lo]
  if (result.data.size() < 5)
  {
    return std::nullopt;
  }

  SearchResult searchResult;
  searchResult.fingerId   = static_cast<uint16_t>((result.data[1] << 8) | result.data[2]);
  searchResult.confidence = static_cast<uint16_t>((result.data[3] << 8) | result.data[4]);

  m_lastSearchResult = searchResult;
  return searchResult;
}

// Fast search
std::optional<SearchResult> FingerprintSensor::fastSearch()
{
  // Refresh system parameters before search like Python does
  (void)getParameters();

  // requireD parameters: [bufferSlot, startID_hi, startID_lo, count_hi, count_lo]
  uint16_t capacity = m_sysParams.capacity;
  if (capacity == 0)
  {
    capacity = 127; // Safe default
  }

  std::array<uint8_t, 5> const data = {
      0x01,                                         // Bufer slot 1
      0x00,                                         // Start ID high byte (0x0000 = start from beginning)
      0x00,                                         // Start ID low byte
      static_cast<uint8_t>((capacity >> 8) & 0xFF), // Count high byte
      static_cast<uint8_t>(capacity & 0xFF)         // Count low byte
  };

  auto result = sendCommandWithData(CommandCode::HighSpeedSearch, data);

  if (result.status != StatusCode::Success)
  {
    return std::nullopt;
  }

  if (result.data.size() < 5)
  {
    return std::nullopt;
  }

  SearchResult searchResult;
  searchResult.fingerId   = static_cast<uint16_t>((result.data[1] << 8) | result.data[2]);
  searchResult.confidence = static_cast<uint16_t>((result.data[3] << 8) | result.data[4]);

  m_lastSearchResult = searchResult;
  return searchResult;
}

StatusCode FingerprintSensor::deleteTemplate(uint16_t startId, uint16_t count)
{
  std::array<uint8_t, 4> const data = {static_cast<uint8_t>((startId >> 8) & 0xFF),
                                       static_cast<uint8_t>(startId & 0xFF), static_cast<uint8_t>((count >> 8) & 0xFF),
                                       static_cast<uint8_t>(count & 0xFF)};
  return sendCommand(CommandCode::Delete, data);
}

StatusCode FingerprintSensor::clearDatabase()
{
  return sendCommand(CommandCode::Empty);
}

std::optional<uint16_t> FingerprintSensor::getTemplateCount()
{
  auto result = sendCommandWithData(CommandCode::TemplateCount, {});

  if (result.status != StatusCode::Success)
  {
    return std::nullopt;
  }

  if (result.data.size() < 3)
  {
    return std::nullopt;
  }

  return static_cast<uint16_t>((result.data[1] << 8) | result.data[2]);
}

std::optional<std::vector<uint16_t>> FingerprintSensor::fetchTemplates()
{
  std::vector<uint16_t> templates;

  // Get system parameters to know library size
  if (!getParameters())
  {
    return std::nullopt;
  }

  // Calculate number of pages (each page = 256 template slots represented as 32-byte bitmap)
  uint16_t librarySize = m_sysParams.capacity;
  uint16_t numPages    = (librarySize + 255) / 256; // Ceiling division

  for (uint16_t page = 0; page < numPages; ++page)
  {
    std::array<uint8_t, 1> const data   = {static_cast<uint8_t>(page)};
    auto                         result = sendCommandWithData(CommandCode::ReadIndexTable, data);

    if (result.status != StatusCode::Success)
    {
      return std::nullopt;
    }

    // Response: [status, 32 bytes of bitmap]
    // Need at least 33 bytes (1 status + 32 bitmap)
    if (result.data.size() < 33)
    {
      continue;
    }

    // Parse bitmap starting at index 1 (skip status byte)
    for (size_t i = 1; i <= 32; ++i)
    {
      uint8_t byte = result.data[i];

      // Check each bit in the byte
      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        if (byte & (1 << bit))
        {
          // page_offset + byte_offset + bit_offset
          uint16_t templateId = (page * 256) + ((i - 1) * 8) + bit;
          if (templateId < librarySize)
          {
            templates.push_back(templateId);
          }
        }
      }
    }
  }

  return templates;
}

StatusCode FingerprintSensor::controlLed(LedMode mode, uint8_t speed, LedColor color, uint8_t cycles)
{
  std::array<uint8_t, 4> const data = {static_cast<uint8_t>(mode), speed, static_cast<uint8_t>(color), cycles};
  return sendCommand(CommandCode::AuraLedConfig, data);
}

StatusCode FingerprintSensor::turnLedOn()
{
  return sendCommand(CommandCode::LedOn);
}

StatusCode FingerprintSensor::turnLedOff()
{
  return sendCommand(CommandCode::LedOff);
}

StatusCode FingerprintSensor::setBaudRate(BaudRateRegister baud)
{
  std::array<uint8_t, 2> const data   = {0x04, // Baud rate register index
                                         static_cast<uint8_t>(baud)};
  auto                         status = sendCommand(CommandCode::WriteSysParam, data);

  if (status == StatusCode::Success)
  {
    // Convert register enum to actual baud rate
    int32_t actualBaud = 0;
    switch (baud)
    {
      case BaudRateRegister::_9600:
        actualBaud = 9600;
        break;
      case BaudRateRegister::_19200:
        actualBaud = 19200;
        break;
      case BaudRateRegister::_28800:
        actualBaud = 28800;
        break;
      case BaudRateRegister::_38400:
        actualBaud = 38400;
        break;
      case BaudRateRegister::_48000:
        actualBaud = 48000;
        break;
      case BaudRateRegister::_57600:
        actualBaud = 57600;
        break;
      case BaudRateRegister::_67200:
        actualBaud = 67200;
        break;
      case BaudRateRegister::_76800:
        actualBaud = 76800;
        break;
      case BaudRateRegister::_86400:
        actualBaud = 86400;
        break;
      case BaudRateRegister::_96000:
        actualBaud = 96000;
        break;
      case BaudRateRegister::_105600:
        actualBaud = 105600;
        break;
      case BaudRateRegister::_115200:
        actualBaud = 115200;
        break;
    }

    if (actualBaud > 0 && m_serial && m_serial->isOpen())
    {
      // Update the host serial port baud rate to match the sensor
      m_serial->setBaudRate(actualBaud);
      QThread::msleep(100); // Small delay for baud rate change to take effect
      qDebug() << "Updated serial port baud rate to" << actualBaud;
    }

    // Refresh system parameters to update cached values
    (void)getParameters();
  }

  return status;
}

StatusCode FingerprintSensor::setSecurityLevelRegister(SecurityLevelRegister level)
{
  std::array<uint8_t, 2> const data   = {0x05, // Security level register index
                                         static_cast<uint8_t>(level)};
  auto                         status = sendCommand(CommandCode::WriteSysParam, data);

  if (status == StatusCode::Success)
  {
    // Refresh system parameters to update cached security level
    (void)getParameters();
  }

  return status;
}

StatusCode FingerprintSensor::setDataPacketSizeRegister(DataPacketSizeRegister packetSize)
{
  std::array<uint8_t, 2> const data   = {0x06, // Data packet size register index
                                         static_cast<uint8_t>(packetSize)};
  auto                         status = sendCommand(CommandCode::WriteSysParam, data);

  if (status == StatusCode::Success)
  {
    // Refresh system parameters to update cached packet length
    // This is critical for data transfer operations
    (void)getParameters();
  }

  return status;
}

StatusCode FingerprintSensor::softReset()
{
  auto status = sendCommand(CommandCode::SoftReset);

  if (status == StatusCode::Success)
  {
    // After reset, wait for sensor to stabilize
    QThread::msleep(500);

    // Refresh system parameters as reset may restore defaults
    (void)getParameters();
  }

  return status;
}

[[nodiscard]] CommandResult FingerprintSensor::sendCommandWithData(CommandCode cmd, std::span<const std::uint8_t> data)
{
  if (m_serial && m_serial->isOpen())
  {
    qint64 staleBytes = m_serial->bytesAvailable();
    if (staleBytes > 0)
    {
      //QByteArray stale = m_serial->readAll();
      //qDebug() << "WARNING: Cleared" << staleBytes << "stale bytes from buffer:" << stale.toHex(' ');
    }
  }

  Packet packet;
  packet.id = PacketId::Command;

  packet.address = {
      static_cast<uint8_t>((m_address >> 24) & 0xFFu),
      static_cast<uint8_t>((m_address >> 16) & 0xFFu),
      static_cast<uint8_t>((m_address >> 8) & 0xFFu),
      static_cast<uint8_t>((m_address)&0xFFu),
  };

  packet.data.push_back(static_cast<uint8_t>(cmd));
  if (!data.empty())
  {
    packet.data.insert(packet.data.end(), data.begin(), data.end());
  }

  //qDebug() << "Sending command:" << QString("0x%1").arg(static_cast<int>(cmd), 2, 16, QChar('0'))
  //         << "with data:" << QString::fromStdString(bytesToHex(packet.data));

  if (!writePacket(packet))
  {
    //qDebug() << "Failed to write packet";
    return {StatusCode::Timeout, {}};
  }

  auto const response = receivePacket();
  if (!response)
  {
    //qDebug() << "No response received";
    return {StatusCode::PacketReceiveError, {}};
  }

  if (response->id != PacketId::Acknowledge)
  {
    //qDebug() << "Response type is not ACK:" << QString("0x%1").arg(static_cast<int>(response->type), 2, 16, QChar('0'));
    return {StatusCode::PacketReceiveError, {}};
  }

  if (response->data.empty())
  {
    //qDebug() << "Response data is empty";
    return {StatusCode::PacketReceiveError, {}};
  }

  auto status = static_cast<StatusCode>(response->data[0]);
  //if (status != StatusCode::Success)
  //{
  //qDebug() << "Command returned status:" << QString("0x%1").arg(static_cast<int>(status), 2, 16, QChar('0'));
  //}

  return {status, response->data};
}

[[nodiscard]] StatusCode FingerprintSensor::sendCommand(CommandCode cmd, std::span<const std::uint8_t> data)
{
  return sendCommandWithData(cmd, data).status;
}

std::optional<FingerprintSensor::Packet> FingerprintSensor::receivePacket(int32_t timeoutMs)
{
  if (!m_serial || !m_serial->isOpen())
  {
    //qDebug() << "Serial port not open";
    return std::nullopt;
  }

  // Read minimum packet size first (header + minimum payload)
  // ACK packet: 9 (header) + 1 (status) + 2 (checksum) = 12 bytes
  static constexpr qint64 MIN_PACKET_SIZE = 12;

  QByteArray buffer;

  // Wait for and read at least the minimum packet
  if (!readExact(m_serial.get(), MIN_PACKET_SIZE, timeoutMs, buffer))
  {
    //qDebug() << "Failed to read minimum packet";
    return std::nullopt;
  }

  //qDebug() << "Received packet:" << buffer.toHex(' ');

  // Parse header from buffer
  uint16_t startCode = static_cast<uint16_t>((static_cast<uint8_t>(buffer[0]) << 8) | static_cast<uint8_t>(buffer[1]));
  if (startCode != START_CODE)
  {
    //qDebug() << "Invalid start code:" << QString("0x%1").arg(startCode, 4, 16, QChar('0'));
    return std::nullopt;
  }

  Packet packet;
  packet.startCode = startCode;

  // Address (bytes 2-5)
  for (size_t i = 0; i < 4; ++i)
  {
    packet.address[i] = static_cast<uint8_t>(buffer[2 + i]);
  }

  // Validate address - strict like Python
  uint32_t received_addr = (static_cast<uint32_t>(packet.address[0]) << 24) |
                           (static_cast<uint32_t>(packet.address[1]) << 16) |
                           (static_cast<uint32_t>(packet.address[2]) << 8) | static_cast<uint32_t>(packet.address[3]);

  if (received_addr != m_address)
  {
    //qDebug() << "Address mismatch! Expected:" << QString("0x%1").arg(m_address, 8, 16, QChar('0'))
    //         << "Received:" << QString("0x%1").arg(received_addr, 8, 16, QChar('0'));
    return std::nullopt; // Reject packet like Python does
  }

  // Type (byte 6)
  uint8_t commandId = static_cast<uint8_t>(buffer[6]);
  packet.id         = static_cast<PacketId>(commandId);

  // Length (bytes 7-8)
  uint16_t length = static_cast<uint16_t>((static_cast<uint8_t>(buffer[7]) << 8) | static_cast<uint8_t>(buffer[8]));
  packet.length   = length;

  if (length < 2)
  {
    //qDebug() << "Invalid packet length:" << length;
    return std::nullopt;
  }

  // Calculate total expected packet size
  qint64 totalSize = PACKET_HEADER_SIZE + length; // 9 + (data + checksum)

  if (buffer.size() < totalSize)
  {
    qint64     remaining = totalSize - buffer.size();
    QByteArray extra;
    if (!readExact(m_serial.get(), remaining, timeoutMs, extra))
    {
      //qDebug() << "Failed to read remaining" << remaining << "bytes";
      return std::nullopt;
    }
    buffer.append(extra);
    //qDebug() << "Read additional:" << extra.toHex(' ');
  }

  // Extract data (everything between header and checksum)
  qint64 dataLen = length - 2; // length includes checksum
  if (dataLen > 0)
  {
    packet.data.resize(static_cast<size_t>(dataLen));
    for (qint64 i = 0; i < dataLen; ++i)
    {
      packet.data[i] = static_cast<uint8_t>(buffer[PACKET_HEADER_SIZE + i]);
    }
  }

  // Extract checksum (last 2 bytes)
  uint8_t  chk_hi           = static_cast<uint8_t>(buffer[PACKET_HEADER_SIZE + dataLen]);
  uint8_t  chk_lo           = static_cast<uint8_t>(buffer[PACKET_HEADER_SIZE + dataLen + 1]);
  uint16_t receivedChecksum = static_cast<uint16_t>((chk_hi << 8) | chk_lo);

  // Calculate checksum: type + length_hi + length_lo + all data bytes
  uint16_t calculatedChecksum = 0;
  calculatedChecksum += commandId;
  calculatedChecksum += static_cast<uint8_t>(length >> 8);
  calculatedChecksum += static_cast<uint8_t>(length & 0xFF);
  for (auto b : packet.data)
  {
    calculatedChecksum += b;
  }

  if (receivedChecksum != calculatedChecksum)
  {
    //qDebug() << "Checksum mismatch! Received:" << QString("0x%1").arg(receivedChecksum, 4, 16, QChar('0'))
    //         << "Calculated:" << QString("0x%1").arg(calculatedChecksum, 4, 16, QChar('0'));
    return std::nullopt;
  }

  //qDebug() << "Packet valid, status:" << QString("0x%1").arg(packet.data.empty() ? 0 : packet.data[0], 2, 16, QChar('0'));
  return packet;
}

bool FingerprintSensor::writePacket(Packet const &packet)
{
  if (!m_serial || !m_serial->isOpen())
  {
    //qDebug() << "Serial port not open for writing";
    return false;
  }

  auto const buffer = packet.serialize();
  //qDebug() << "Writing packet:" << QString::fromStdString(bytesToHex(buffer));

  qint64 const total   = static_cast<qint64>(buffer.size());
  qint64       written = 0;

  // Write in a loop to handle partial writes (rare but safe)
  while (written < total)
  {
    qint64 chunk = m_serial->write(reinterpret_cast<char const *>(buffer.data() + written), total - written);
    if (chunk <= 0)
    {
      //qDebug() << "Failed to write data, error:" << m_serial->errorString();
      return false;
    }
    written += chunk;
    if (!m_serial->waitForBytesWritten(DEFAULT_TIMEOUT_MS))
    {
      //qDebug() << "Write timeout";
      return false;
    }
  }

  return true;
}

StatusCode FingerprintSensor::loadTemplate(uint16_t templateId, uint8_t bufferSlot)
{
  std::array<uint8_t, 3> const data = {bufferSlot, static_cast<uint8_t>((templateId >> 8) & 0xFF),
                                       static_cast<uint8_t>(templateId & 0xFF)};
  return sendCommand(CommandCode::Load, data);
}

std::optional<std::vector<uint8_t>> FingerprintSensor::receiveDataPackets(int32_t timeoutMs)
{
  std::vector<uint8_t> allData;

  while (true)
  {
    auto packet = receivePacket(timeoutMs);
    if (!packet)
    {
      //qDebug() << "Failed to receive data packet";
      return std::nullopt;
    }

    if (packet->id != PacketId::Data && packet->id != PacketId::EndData)
    {
      //qDebug() << "Unexpected packet type:" << static_cast<int>(packet->type);
      return std::nullopt;
    }

    allData.insert(allData.end(), packet->data.begin(), packet->data.end());

    if (packet->id == PacketId::EndData)
    {
      break;
    }
  }

  return allData;
}

bool FingerprintSensor::sendDataPackets(std::span<const uint8_t> data)
{
  size_t packetSize = m_sysParams.packetLength;
  if (packetSize == 0)
  {
    packetSize = 128;
  }

  size_t totalSize = data.size();
  size_t offset    = 0;

  while (offset < totalSize)
  {
    size_t chunkSize    = std::min(packetSize, totalSize - offset);
    bool   isLastPacket = (offset + chunkSize >= totalSize);

    Packet packet;
    packet.id      = isLastPacket ? PacketId::EndData : PacketId::Data;
    packet.address = {static_cast<uint8_t>((m_address >> 24) & 0xFF), static_cast<uint8_t>((m_address >> 16) & 0xFF),
                      static_cast<uint8_t>((m_address >> 8) & 0xFF), static_cast<uint8_t>(m_address & 0xFF)};

    packet.data.assign(data.begin() + offset, data.begin() + offset + chunkSize);

    if (!writePacket(packet))
    {
      return false;
    }

    offset += chunkSize;
  }

  return true;
}

std::optional<std::vector<uint8_t>> FingerprintSensor::downloadTemplate(uint8_t bufferSlot)
{
  std::array<uint8_t, 1> const cmdData = {bufferSlot};
  auto                         result  = sendCommandWithData(CommandCode::Upload, cmdData);

  if (result.status != StatusCode::Success)
  {
    //qDebug() << "Upload template command failed";
    return std::nullopt;
  }

  return receiveDataPackets();
}

StatusCode FingerprintSensor::uploadTemplate(std::span<const uint8_t> data, uint8_t bufferSlot)
{
  std::array<uint8_t, 1> const cmdData = {bufferSlot};
  auto                         result  = sendCommandWithData(CommandCode::Download, cmdData);

  if (result.status != StatusCode::Success)
  {
    return result.status;
  }

  if (!sendDataPackets(data))
  {
    return StatusCode::UploadFailed;
  }

  return StatusCode::Success;
}

std::optional<std::vector<uint8_t>> FingerprintSensor::downloadImage()
{
  auto result = sendCommandWithData(static_cast<CommandCode>(0x0A), {});

  if (result.status != StatusCode::Success)
  {
    //qDebug() << "Upload image command failed";
    return std::nullopt;
  }

  return receiveDataPackets();
}

StatusCode FingerprintSensor::uploadImage(std::span<const uint8_t> data)
{
  auto result = sendCommandWithData(static_cast<CommandCode>(0x0B), {});

  if (result.status != StatusCode::Success)
  {
    return result.status;
  }

  if (!sendDataPackets(data))
  {
    return StatusCode::UploadFailed;
  }

  return StatusCode::Success;
}

} // namespace carbio