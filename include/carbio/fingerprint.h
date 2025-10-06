#ifndef CARBIO_FINGERPRINT_SENSOR_H
#define CARBIO_FINGERPRINT_SENSOR_H

#include <QSerialPort>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace carbio
{
enum class StatusCode : uint8_t
{
  Success                 = 0x00, ///< Command execution is complete
  PacketReceiveError      = 0x01, ///< Error when receving data package
  NoFinger                = 0x02, ///< No finger was on sensor
  ImageCaptureFailed      = 0x03, ///< Failed capturing fingerprint image
  ImageQualityPoor        = 0x06, ///< Fingerprint image quality too poor
  FeatureExtractionFailed = 0x07, ///< Failed to extract features
  NoMatch                 = 0x08, ///< Fingerprint doesn't match
  NotFound                = 0x09, ///< Failed to find matching fingerprint
  EnrollMismatch          = 0x0A, ///< Failed to combine character files
  InvalidLocation         = 0x0B, ///< PageID is beyond the library
  DatabaseRangeError      = 0x0C, ///< Error reading template from library
  UploadFeatureFailed     = 0x0D, ///< Error uploading template
  DeviceOrResourceTooBusy = 0x0E, ///< Sensor could not respond to new request
  UploadFailed            = 0x0F, ///< Error uploading image
  DeleteFailed            = 0x10, ///< Failed to delete template
  DatabaseClearFailed     = 0x11, ///< Failed to clear fingerprint library
  PermissionDenied        = 0x13, ///< Password verification failed
  InvalidImage            = 0x15, ///< No valid primary image
  FlashError              = 0x18, ///< Error writing to flash memory
  InvalidRegister         = 0x1A, ///< Invalid register number
  Timeout                 = 0xFF, ///< Operation timeout
  BadPacket               = 0xFE  ///< Malformed packet received
};

enum class PacketId : uint8_t
{
  Command     = 0x01, ///< Command packet
  Data        = 0x02, ///< Data packet
  Acknowledge = 0x07, ///< Acknowledge packet
  EndData     = 0x08  ///< End of data packet
};

enum class CommandCode : uint8_t
{
  GetImage        = 0x01, ///< Capture fingerprint image
  Image2Tz        = 0x02, ///< Convert image to template
  Search          = 0x04, ///< Search for fingerprint
  RegModel        = 0x05, ///< Generate template from characteristics
  Store           = 0x06, ///< Store template
  Load            = 0x07, ///< Load template
  Upload          = 0x08, ///< Upload template
  Download        = 0x09, ///< Download template
  Delete          = 0x0C, ///< Delete templates
  Empty           = 0x0D, ///< Clear database
  WriteSysParam   = 0x0E, ///< Write system parameters
  ReadSysParam    = 0x0F, ///< Read system parameters
  SetPassword     = 0x12, ///< Set password
  VerifyPassword  = 0x13, ///< Verify password
  HighSpeedSearch = 0x1B, ///< High-speed search
  TemplateCount   = 0x1D, ///< Get template count
  ReadIndexTable  = 0x1F, ///< Read template index table
  AuraLedConfig   = 0x35, ///< Configure LED
  SoftReset       = 0x3D, ///< Soft reset device
  LedOn           = 0x50, ///< Turn on LED
  LedOff          = 0x51  ///< Turn off LED
};

enum class LedMode : uint8_t
{
  Breathing  = 0x01, ///< Breathing effect
  Flashing   = 0x02, ///< Flashing effect
  On         = 0x03, ///< Always on
  Off        = 0x04, ///< Always off
  GradualOn  = 0x05, ///< Fade in
  GradualOff = 0x06  ///< Fade out
};

enum class LedColor : uint8_t
{
  Red    = 0x01,
  Blue   = 0x02,
  Purple = 0x03,
  Green  = 0x04,
  Yellow = 0x05,
  Cyan   = 0x06,
  White  = 0x07
};

enum class BaudRateRegister : uint8_t
{
  _9600   = 0x01,
  _19200  = 0x02,
  _28800  = 0x03,
  _38400  = 0x04,
  _48000  = 0x05,
  _57600  = 0x06,
  _67200  = 0x07,
  _76800  = 0x08,
  _86400  = 0x09,
  _96000  = 0x0A,
  _105600 = 0x0B,
  _115200 = 0x0C
};

enum class SecurityLevelRegister : uint8_t
{
  VeryLow  = 0x01,
  Low      = 0x02,
  Balanced = 0x03,
  High     = 0x04,
  VeryHigh = 0x05
};

enum class DataPacketSizeRegister : std::uint8_t
{
  _32  = 0x00,
  _64  = 0x01,
  _128 = 0x02,
  _256 = 0x03
};

struct SystemParameters
{
  uint16_t statusRegister{0};
  uint16_t systemId{0};
  uint16_t capacity{127};
  uint16_t SecurityLevel{0};
  uint32_t deviceAddress{0xFFFFFFFF};
  uint16_t packetLength{128};
  uint16_t baudRate{57600};
};

struct SearchResult
{
  uint16_t fingerId{0};   ///< Matched template ID
  uint16_t confidence{0}; ///< Confidence score (0-255)
};

struct CommandResult
{
  StatusCode           status;
  std::vector<uint8_t> data;
};

class FingerprintSensor
{
public:
  static constexpr uint16_t START_CODE         = 0xEF01;
  static constexpr uint32_t DEFAULT_ADDRESS    = 0xFFFFFFFF;
  static constexpr int32_t  DEFAULT_TIMEOUT_MS = 1000;
  static constexpr size_t   MAX_PACKET_SIZE    = 256;
  static constexpr size_t   PACKET_HEADER_SIZE = 9;

  explicit FingerprintSensor(std::string_view devicePath);
  ~FingerprintSensor();

  FingerprintSensor(FingerprintSensor const &)            = delete;
  FingerprintSensor &operator=(FingerprintSensor const &) = delete;

  FingerprintSensor(FingerprintSensor &&) noexcept;
  FingerprintSensor &operator=(FingerprintSensor &&) noexcept;

  [[nodiscard]] bool begin();
  [[nodiscard]] bool begin(int32_t baudRate);
  void               end();

  [[nodiscard]] StatusCode                      verifyPassword();
  [[nodiscard]] std::optional<SystemParameters> getParameters();
  [[nodiscard]] StatusCode                      captureImage();
  [[nodiscard]] StatusCode                      imageToTemplate(uint8_t bufferSlot = 1);
  [[nodiscard]] StatusCode                      createModel();
  [[nodiscard]] StatusCode                      storeTemplate(uint16_t templateId, uint8_t bufferSlot = 1);
  [[nodiscard]] std::optional<SearchResult> search(uint8_t bufferSlot = 1, uint16_t startId = 0, uint16_t count = 0);
  [[nodiscard]] std::optional<SearchResult> fastSearch();
  [[nodiscard]] StatusCode                  deleteTemplate(uint16_t startId, uint16_t count = 1);
  [[nodiscard]] StatusCode                  clearDatabase();
  [[nodiscard]] std::optional<uint16_t>     getTemplateCount();
  [[nodiscard]] std::optional<std::vector<uint16_t>> fetchTemplates();
  [[nodiscard]] StatusCode                           loadTemplate(uint16_t templateId, uint8_t bufferSlot = 1);
  [[nodiscard]] std::optional<std::vector<uint8_t>>  downloadTemplate(uint8_t bufferSlot = 1);
  [[nodiscard]] StatusCode uploadTemplate(std::span<const uint8_t> data, uint8_t bufferSlot = 1);
  [[nodiscard]] std::optional<std::vector<uint8_t>> downloadImage();
  [[nodiscard]] StatusCode                          uploadImage(std::span<const uint8_t> data);
  [[nodiscard]] StatusCode controlLed(LedMode mode, uint8_t speed = 128, LedColor color = LedColor::Blue,
                                      uint8_t cycles = 0);
  [[nodiscard]] StatusCode turnLedOn();
  [[nodiscard]] StatusCode turnLedOff();
  [[nodiscard]] StatusCode setBaudRate(BaudRateRegister baud);
  [[nodiscard]] StatusCode setSecurityLevelRegister(SecurityLevelRegister level);
  [[nodiscard]] StatusCode setDataPacketSizeRegister(DataPacketSizeRegister packetSize);
  [[nodiscard]] StatusCode softReset();
  [[nodiscard]] std::optional<SearchResult> getLastSearchResult() const
  {
    return m_lastSearchResult;
  }
  [[nodiscard]] SystemParameters const &getCachedParameters() const
  {
    return m_sysParams;
  }

private:
  struct Packet
  {
    uint16_t               startCode{START_CODE};
    std::array<uint8_t, 4> address{0xFF, 0xFF, 0xFF, 0xFF};
    PacketId               id{PacketId::Command};
    uint16_t               length{0};
    std::vector<uint8_t>   data{};

    [[nodiscard]] std::vector<uint8_t> serialize() const;
  };

  [[nodiscard]] CommandResult         sendCommandWithData(CommandCode cmd, std::span<const std::uint8_t> data = {});
  [[nodiscard]] StatusCode            sendCommand(CommandCode cmd, std::span<const std::uint8_t> data = {});
  [[nodiscard]] std::optional<Packet> receivePacket(int32_t timeoutMs = DEFAULT_TIMEOUT_MS);
  [[nodiscard]] bool                  writePacket(Packet const &packet);
  [[nodiscard]] std::optional<std::vector<uint8_t>> receiveDataPackets(int32_t timeoutMs = DEFAULT_TIMEOUT_MS);
  [[nodiscard]] bool                                sendDataPackets(std::span<const uint8_t> data);

  std::unique_ptr<QSerialPort> m_serial;
  std::string_view             m_portName;
  std::uint32_t                m_password;
  std::uint32_t                m_address;
  SystemParameters             m_sysParams;
  std::optional<SearchResult>  m_lastSearchResult;
};

} // namespace carbio

#endif // CARBIO_FINGERPRINT_SENSOR_H
