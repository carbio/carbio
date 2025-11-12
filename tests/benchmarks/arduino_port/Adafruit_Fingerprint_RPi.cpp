/**
 * Adafruit Fingerprint Sensor Library - Raspberry Pi Port Implementation
 *
 * This is a port of the Arduino Adafruit_Fingerprint library to Raspberry Pi/Linux.
 * Original Arduino library: https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
 *
 * Ported for performance benchmarking comparison.
 */

#include "Adafruit_Fingerprint_RPi.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>

// Helper macros from Arduino library
#define GET_CMD_PACKET(...) \
    uint8_t data[] = {__VA_ARGS__}; \
    Adafruit_Fingerprint_Packet packet; \
    packet.start_code = FINGERPRINT_STARTCODE; \
    packet.address[0] = 0xFF; \
    packet.address[1] = 0xFF; \
    packet.address[2] = 0xFF; \
    packet.address[3] = 0xFF; \
    packet.type = FINGERPRINT_COMMANDPACKET; \
    packet.length = sizeof(data); \
    memcpy(packet.data, data, sizeof(data)); \
    writeStructuredPacket(packet); \
    if (getStructuredPacket(&packet) != FINGERPRINT_OK) \
        return FINGERPRINT_PACKETRECIEVEERR; \
    if (packet.type != FINGERPRINT_ACKPACKET) \
        return FINGERPRINT_PACKETRECIEVEERR;

#define SEND_CMD_PACKET(...) \
    GET_CMD_PACKET(__VA_ARGS__); \
    return packet.data[0];

// Constructor
Adafruit_Fingerprint_RPi::Adafruit_Fingerprint_RPi(const char* serial_port, uint32_t password)
    : fd(-1), port_name(serial_port), thePassword(password), theAddress(0xFFFFFFFF),
      fingerID(0), confidence(0), templateCount(0), status_reg(0), system_id(0),
      capacity(64), security_level(0), device_addr(0xFFFFFFFF), packet_len(64),
      baud_rate(57600)
{
    memset(recvPacket, 0, sizeof(recvPacket));
}

// Destructor
Adafruit_Fingerprint_RPi::~Adafruit_Fingerprint_RPi()
{
    end();
}

// Initialize serial connection
bool Adafruit_Fingerprint_RPi::begin(uint32_t baud_rate_val)
{
    baud_rate = baud_rate_val;

    // Open serial port
    fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        return false;
    }

    // Configure serial port
    struct termios options;
    tcgetattr(fd, &options);

    // Set baud rate
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        default:     speed = B57600; break;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // No hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Enable receiver, ignore modem control lines
    options.c_cflag |= CREAD | CLOCAL;

    // Raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Raw output
    options.c_oflag &= ~OPOST;

    // No input processing
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(INLCR | ICRNL);

    // Set timeouts
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10; // 1 second timeout

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);

    // Give sensor time to boot
    usleep(1000000); // 1 second

    return true;
}

// Close connection
void Adafruit_Fingerprint_RPi::end()
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

// Verify password
bool Adafruit_Fingerprint_RPi::verifyPassword()
{
    return checkPassword() == FINGERPRINT_OK;
}

uint8_t Adafruit_Fingerprint_RPi::checkPassword()
{
    GET_CMD_PACKET(FINGERPRINT_VERIFYPASSWORD,
                   (uint8_t)(thePassword >> 24),
                   (uint8_t)(thePassword >> 16),
                   (uint8_t)(thePassword >> 8),
                   (uint8_t)(thePassword & 0xFF));

    if (packet.data[0] == FINGERPRINT_OK)
        return FINGERPRINT_OK;
    else
        return FINGERPRINT_PACKETRECIEVEERR;
}

// Get system parameters
uint8_t Adafruit_Fingerprint_RPi::getParameters()
{
    GET_CMD_PACKET(FINGERPRINT_READSYSPARAM);

    status_reg = ((uint16_t)packet.data[1] << 8) | packet.data[2];
    system_id = ((uint16_t)packet.data[3] << 8) | packet.data[4];
    capacity = ((uint16_t)packet.data[5] << 8) | packet.data[6];
    security_level = ((uint16_t)packet.data[7] << 8) | packet.data[8];
    device_addr = ((uint32_t)packet.data[9] << 24) | ((uint32_t)packet.data[10] << 16) |
                  ((uint32_t)packet.data[11] << 8) | (uint32_t)packet.data[12];
    packet_len = ((uint16_t)packet.data[13] << 8) | packet.data[14];
    baud_rate = (((uint16_t)packet.data[15] << 8) | packet.data[16]) * 9600;

    return packet.data[0];
}

// Get fingerprint image
uint8_t Adafruit_Fingerprint_RPi::getImage()
{
    SEND_CMD_PACKET(FINGERPRINT_GETIMAGE);
}

// Convert image to template
uint8_t Adafruit_Fingerprint_RPi::image2Tz(uint8_t slot)
{
    SEND_CMD_PACKET(FINGERPRINT_IMAGE2TZ, slot);
}

// Create fingerprint model
uint8_t Adafruit_Fingerprint_RPi::createModel()
{
    SEND_CMD_PACKET(FINGERPRINT_REGMODEL);
}

// Empty database
uint8_t Adafruit_Fingerprint_RPi::emptyDatabase()
{
    SEND_CMD_PACKET(FINGERPRINT_EMPTY);
}

// Store model
uint8_t Adafruit_Fingerprint_RPi::storeModel(uint16_t id)
{
    SEND_CMD_PACKET(FINGERPRINT_STORE, 0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF));
}

// Load model
uint8_t Adafruit_Fingerprint_RPi::loadModel(uint16_t id)
{
    SEND_CMD_PACKET(FINGERPRINT_LOAD, 0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF));
}

// Delete model
uint8_t Adafruit_Fingerprint_RPi::deleteModel(uint16_t id)
{
    SEND_CMD_PACKET(FINGERPRINT_DELETE, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF), 0x00, 0x01);
}

// Fast search
uint8_t Adafruit_Fingerprint_RPi::fingerFastSearch()
{
    GET_CMD_PACKET(FINGERPRINT_HISPEEDSEARCH, 0x01, 0x00, 0x00,
                   (uint8_t)(capacity >> 8), (uint8_t)(capacity & 0xFF));

    fingerID = 0xFFFF;
    confidence = 0xFFFF;

    fingerID = ((uint16_t)packet.data[1] << 8) | packet.data[2];
    confidence = ((uint16_t)packet.data[3] << 8) | packet.data[4];

    return packet.data[0];
}

// Regular search
uint8_t Adafruit_Fingerprint_RPi::fingerSearch(uint8_t slot)
{
    GET_CMD_PACKET(FINGERPRINT_SEARCH, slot, 0x00, 0x00,
                   (uint8_t)(capacity >> 8), (uint8_t)(capacity & 0xFF));

    fingerID = 0xFFFF;
    confidence = 0xFFFF;

    fingerID = ((uint16_t)packet.data[1] << 8) | packet.data[2];
    confidence = ((uint16_t)packet.data[3] << 8) | packet.data[4];

    return packet.data[0];
}

// Get template count
uint8_t Adafruit_Fingerprint_RPi::getTemplateCount()
{
    GET_CMD_PACKET(FINGERPRINT_TEMPLATECOUNT);

    templateCount = ((uint16_t)packet.data[1] << 8) | packet.data[2];

    return packet.data[0];
}

// Set password
uint8_t Adafruit_Fingerprint_RPi::setPassword(uint32_t password)
{
    SEND_CMD_PACKET(FINGERPRINT_SETPASSWORD,
                    (uint8_t)(password >> 24),
                    (uint8_t)(password >> 16),
                    (uint8_t)(password >> 8),
                    (uint8_t)(password & 0xFF));
}

// LED control (simple)
uint8_t Adafruit_Fingerprint_RPi::LEDcontrol(bool on)
{
    if (on) {
        SEND_CMD_PACKET(FINGERPRINT_LEDON);
    } else {
        SEND_CMD_PACKET(FINGERPRINT_LEDOFF);
    }
}

// LED control (advanced)
uint8_t Adafruit_Fingerprint_RPi::LEDcontrol(uint8_t control, uint8_t speed, uint8_t coloridx, uint8_t count)
{
    SEND_CMD_PACKET(FINGERPRINT_AURALEDCONFIG, control, speed, coloridx, count);
}

// Set baud rate
uint8_t Adafruit_Fingerprint_RPi::setBaudRate(uint8_t baudrate)
{
    SEND_CMD_PACKET(FINGERPRINT_WRITE_REG, FINGERPRINT_BAUD_REG_ADDR, baudrate);
}

// Set security level
uint8_t Adafruit_Fingerprint_RPi::setSecurityLevel(uint8_t level)
{
    SEND_CMD_PACKET(FINGERPRINT_WRITE_REG, FINGERPRINT_SECURITY_REG_ADDR, level);
}

// Set packet size
uint8_t Adafruit_Fingerprint_RPi::setPacketSize(uint8_t size)
{
    SEND_CMD_PACKET(FINGERPRINT_WRITE_REG, FINGERPRINT_PACKET_REG_ADDR, size);
}

uint8_t Adafruit_Fingerprint_RPi::getModel()
{
    SEND_CMD_PACKET(FINGERPRINT_UPLOAD, 0x01);
}

uint8_t Adafruit_Fingerprint_RPi::writeRegister(uint8_t regAdd, uint8_t value)
{
    SEND_CMD_PACKET(FINGERPRINT_WRITE_REG, regAdd, value);
}

// Write packet to sensor
void Adafruit_Fingerprint_RPi::writeStructuredPacket(const Adafruit_Fingerprint_Packet& packet)
{
    uint8_t buf[128];
    int idx = 0;

    buf[idx++] = (uint8_t)(packet.start_code >> 8);
    buf[idx++] = (uint8_t)(packet.start_code & 0xFF);
    buf[idx++] = packet.address[0];
    buf[idx++] = packet.address[1];
    buf[idx++] = packet.address[2];
    buf[idx++] = packet.address[3];
    buf[idx++] = packet.type;

    uint16_t wire_length = packet.length + 2;
    buf[idx++] = (uint8_t)(wire_length >> 8);
    buf[idx++] = (uint8_t)(wire_length & 0xFF);

    uint16_t sum = (packet.type >> 8) + (packet.type & 0xFF) +
                   (wire_length >> 8) + (wire_length & 0xFF);

    for (uint16_t i = 0; i < packet.length; i++) {
        buf[idx++] = packet.data[i];
        sum += packet.data[i];
    }

    buf[idx++] = (uint8_t)(sum >> 8);
    buf[idx++] = (uint8_t)(sum & 0xFF);

    serial_write(buf, idx);
}

// Read packet from sensor
uint8_t Adafruit_Fingerprint_RPi::getStructuredPacket(Adafruit_Fingerprint_Packet* packet, uint16_t timeout)
{
    uint8_t byte;
    uint16_t idx = 0;
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (true) {
        // Check timeout
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                         (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= timeout) {
            return FINGERPRINT_TIMEOUT;
        }

        // Try to read a byte
        if (serial_read(&byte, 1, 10) <= 0) {
            continue;
        }

        switch (idx) {
        case 0:
            if (byte != (FINGERPRINT_STARTCODE >> 8))
                continue;
            packet->start_code = (uint16_t)byte << 8;
            break;
        case 1:
            packet->start_code |= byte;
            if (packet->start_code != FINGERPRINT_STARTCODE)
                return FINGERPRINT_BADPACKET;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            packet->address[idx - 2] = byte;
            break;
        case 6:
            packet->type = byte;
            break;
        case 7:
            packet->length = (uint16_t)byte << 8;
            break;
        case 8:
            packet->length |= byte;
            break;
        default:
            packet->data[idx - 9] = byte;
            if ((idx - 8) >= packet->length) {
                return FINGERPRINT_OK;
            }
            break;
        }
        idx++;
    }

    return FINGERPRINT_BADPACKET;
}

// Serial I/O helpers
int Adafruit_Fingerprint_RPi::serial_write(const uint8_t* data, size_t len)
{
    if (fd < 0)
        return -1;
    return write(fd, data, len);
}

int Adafruit_Fingerprint_RPi::serial_read(uint8_t* data, size_t len, uint32_t timeout_ms)
{
    if (fd < 0)
        return -1;

    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(fd + 1, &readfds, NULL, NULL, &timeout);

    if (result > 0 && FD_ISSET(fd, &readfds)) {
        return read(fd, data, len);
    }

    return 0;
}

int Adafruit_Fingerprint_RPi::serial_available()
{
    int bytes = 0;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}

void Adafruit_Fingerprint_RPi::serial_flush()
{
    tcflush(fd, TCIOFLUSH);
}
