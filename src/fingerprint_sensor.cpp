#include "carbio/fingerprint_sensor.h"
#include "carbio/command_serializer.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <thread>

namespace carbio
{
fingerprint_sensor::fingerprint_sensor() : serial_(), protocol_(), executor_(serial_, protocol_)
{
}

fingerprint_sensor::~fingerprint_sensor() = default;

fingerprint_sensor::fingerprint_sensor(fingerprint_sensor &&other) noexcept
    : serial_(std::move(other.serial_)), protocol_(std::move(other.protocol_)), executor_(serial_, protocol_)
{
}

fingerprint_sensor &
fingerprint_sensor::operator=(fingerprint_sensor &&other) noexcept
{
  if (this != &other)
  {
    serial_   = std::move(other.serial_);
    protocol_ = std::move(other.protocol_);
    // executor_ cannot be reassigned (contains references),
    // reconstruct through placement new
    executor_.~command_executor();
    new (&executor_) command_executor(serial_, protocol_);
  }
  return *this;
}

// --- connection management

bool
fingerprint_sensor::is_open() const noexcept
{
  return serial_.is_open();
}

bool
fingerprint_sensor::open(std::string_view path) noexcept
{
  // Auto-detect baud rate by trying all common rates in priority order
  // Custom baud rates are supported via Linux ioctl on non-standard rates
  constexpr std::array<std::uint32_t, 12> baud_rates = {
    57600, 115200, 9600, 19200, 28800, 38400,
    48000, 67200, 76800, 86400, 96000, 105600
  };

  for (auto baud : baud_rates)
  {
    // Try opening with this baud rate
    serial_config config;
    config.baud_rate = baud;

    std::fprintf(stderr, "[DEBUG] Trying baud rate: %u\n", baud);

    if (!serial_.open(path, config))
    {
      std::fprintf(stderr, "[DEBUG]   Failed to open port\n");
      continue;
    }

    std::fprintf(stderr, "[DEBUG]   Port opened successfully\n");

    // Wait for sensor to stabilize after opening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Flush any startup bytes from the buffer
    serial_.flush();

    // Perform password handshake - required before sensor accepts commands
    std::fprintf(stderr, "[DEBUG]   Verifying password...\n");
    auto verify_result = verify_device_password();
    if (!verify_result)
    {
      std::fprintf(stderr, "[DEBUG]   Password verification failed: 0x%02x\n", static_cast<int>(verify_result.error()));
      serial_.close();
      continue; // Try next baud rate
    }

    std::fprintf(stderr, "[DEBUG]   Password verified!\n");

    // Read and cache system parameters
    std::fprintf(stderr, "[DEBUG]   Reading system parameters...\n");
    auto params = get_device_setting_info();
    if (!params)
    {
      std::fprintf(stderr, "[DEBUG]   Failed to read parameters: 0x%02x\n", static_cast<int>(params.error()));
      serial_.close();
      continue; // Try next baud rate
    }

    std::fprintf(stderr, "[DEBUG]   Connected at %u baud!\n", baud);

    // Update protocol handler with actual packet length from device
    protocol_.set_packet_length(params->length);

    return true;
  }

  // All baud rates failed
  return false;
}

void
fingerprint_sensor::close() noexcept
{
  serial_.close();
}

// --- device configuration

result<device_setting_info>
fingerprint_sensor::get_device_setting_info() noexcept
{
  return executor_.execute<command_code::read_system_parameter>({});
}

void_result
fingerprint_sensor::set_baud_rate_setting(baud_rate_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{
      static_cast<std::uint8_t>(device_setting_index::baud_rate_setting), static_cast<std::uint8_t>(setting)};
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::set_security_level_setting(security_level_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{
      static_cast<std::uint8_t>(device_setting_index::security_level_setting), static_cast<std::uint8_t>(setting)};
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::set_data_length_setting(data_length_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{
      static_cast<std::uint8_t>(device_setting_index::packet_length_setting), static_cast<std::uint8_t>(setting)};
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::soft_reset_device() noexcept
{
  return executor_.execute<command_code::soft_reset_device>({});
}

// --- led management

void_result
fingerprint_sensor::set_led_setting(carbio::led_mode_setting mode, uint8_t speed, carbio::led_color_setting color, uint8_t cycles) noexcept
{
  return executor_.execute<command_code::set_led_config>(
      {static_cast<std::uint8_t>(mode), speed, static_cast<std::uint8_t>(color), cycles});
}

void_result
fingerprint_sensor::turn_led_on() noexcept
{
  return executor_.execute<command_code::turn_led_on>({});
}

void_result
fingerprint_sensor::turn_led_off() noexcept
{
  return executor_.execute<command_code::turn_led_off>({});
}

// --- security

void_result
fingerprint_sensor::set_device_password(std::uint32_t new_password) noexcept
{
  return executor_.execute<command_code::set_device_password>({new_password});
}

void_result
fingerprint_sensor::verify_device_password() noexcept
{
  // Need password from somewhere - get it from protocol handler
  return executor_.execute<command_code::verify_device_password>({0x00000000});
}

void_result
fingerprint_sensor::set_device_address(std::uint32_t new_address) noexcept
{
  protocol_.set_address(new_address);
  return make_success();
}

// --- low-level ops

void_result
fingerprint_sensor::capture_image() noexcept
{
  return executor_.execute<command_code::capture_image>({});
}

void_result
fingerprint_sensor::extract_features(std::uint8_t buffer_id)
{
  return executor_.execute<command_code::extract_features>(std::array<std::uint8_t, 1>{buffer_id});
}

void_result
fingerprint_sensor::create_model() noexcept
{
  return executor_.execute<command_code::create_model>({});
}

void_result
fingerprint_sensor::store_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  return executor_.execute<command_code::store_model>({buffer_id, page_id});
}

void_result
fingerprint_sensor::load_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  return executor_.execute<command_code::load_model>({buffer_id, page_id});
}

void_result
fingerprint_sensor::upload_model(std::span<const std::uint8_t> buffer, std::uint8_t buffer_id) noexcept
{
  // First send the upload command
  auto result = executor_.execute<command_code::upload_model>({buffer_id});
  if (!result) return make_error(result.error());

  // Then send data packets
  return executor_.send_data_packets(buffer);
}

void_result
fingerprint_sensor::download_model(std::span<std::uint8_t> buffer) noexcept
{
  // First send the download command
  auto result = executor_.execute<command_code::download_model>({1}); // buffer_id = 1
  if (!result) return make_error(result.error());

  // Then receive data packets
  auto data_result = executor_.receive_data_packets();
  if (!data_result) return make_error(data_result.error());

  // Copy to output buffer
  std::size_t copy_size = std::min(buffer.size(), data_result->size());
  std::copy_n(data_result->begin(), copy_size, buffer.begin());

  return make_success();
}

// --- database ops

void_result
fingerprint_sensor::erase_model(std::uint16_t page_id, std::uint16_t count) noexcept
{
  return executor_.execute<command_code::erase_model>({page_id, count});
}

void_result
fingerprint_sensor::clear_database() noexcept
{
  return executor_.execute<command_code::clear_database>({});
}

result<match_query_info>
fingerprint_sensor::match_model() noexcept
{
  return executor_.execute<command_code::match_model>({});
}

result<search_query_info>
fingerprint_sensor::search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  return executor_.execute<command_code::search_model>({buffer_id, page_id, count});
}

result<search_query_info>
fingerprint_sensor::fast_search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  return executor_.execute<command_code::fast_search_model>({buffer_id, page_id, count});
}

result<std::uint16_t>
fingerprint_sensor::model_count() noexcept
{
  return executor_.execute<command_code::count_model>({});
}

result<std::vector<std::uint8_t>>
fingerprint_sensor::read_index_table(std::span<std::uint8_t> data) noexcept
{
  auto result = executor_.execute<command_code::read_index_table>(std::array<std::uint8_t, 1>{0});
  if (!result) return make_error(result.error());

  // Copy to output buffer and return
  std::size_t copy_size = std::min(data.size(), result->size());
  std::copy_n(result->begin(), copy_size, data.begin());

  return make_success(std::vector<std::uint8_t>(result->begin(), result->end()));
}

} /* namespace carbio */
