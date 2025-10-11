#include "carbio/fingerprint_sensor.h"

#include "carbio/command_serializer.h"
#include <spdlog/spdlog.h>

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
  static constexpr std::array<std::uint32_t, 12> baud_rates = {57600, 115200, 9600, 19200, 28800, 38400, 48000, 67200, 76800, 86400, 96000, 105600};
  spdlog::info("attempting to connect sensor...");
  for (auto baud : baud_rates)
  {
    if (!serial_.open(path))
    {
      spdlog::error("failed to open port at {}", path);
      continue;
    }

    // Now set baud rate after port is open
    if (!serial_.set_baud_rate(baud))
    {
      spdlog::error("failed to set baud rate {}", baud);
      serial_.close();
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto device_auth = verify_device_password();
    if (!device_auth)
    {
      spdlog::error("failed device password authentication");
      serial_.close();
      continue; // Try next baud rate
    }
    auto settings = get_device_setting_info();
    if (!settings)
    {
      spdlog::error("failed reading device settings");
      serial_.close();
      continue; // Try next baud rate
    }
    protocol_.set_packet_length(settings->length);
    spdlog::info("connected to sensor successfully!");
    return true;
  }
  // All baud rates failed
  spdlog::error("failed connecting to sensor!");
  return false;
}

void
fingerprint_sensor::close() noexcept
{
  spdlog::info("closing sensor connection...");
  serial_.close();
}

// --- device configuration

result<device_setting_info>
fingerprint_sensor::get_device_setting_info() noexcept
{
  spdlog::debug("getting device settings...");
  return executor_.execute<command_code::read_system_parameter>({});
}

void_result
fingerprint_sensor::set_baud_rate_setting(baud_rate_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{ static_cast<std::uint8_t>(device_setting_index::baud_rate_setting), static_cast<std::uint8_t>(setting)};
  spdlog::debug("setting baud rate...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::set_security_level_setting(security_level_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{ static_cast<std::uint8_t>(device_setting_index::security_level_setting), static_cast<std::uint8_t>(setting)};
  spdlog::debug("setting security level...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::set_data_length_setting(data_length_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{ static_cast<std::uint8_t>(device_setting_index::packet_length_setting), static_cast<std::uint8_t>(setting)};
  spdlog::debug("setting packet data length...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result
fingerprint_sensor::soft_reset_device() noexcept
{
  spdlog::debug("performing soft reset...");
  return executor_.execute<command_code::soft_reset_device>({});
}

// --- led management

void_result
fingerprint_sensor::set_led_setting(carbio::led_mode_setting mode, uint8_t speed, carbio::led_color_setting color, uint8_t cycles) noexcept
{
  spdlog::debug("setting led configuration");
  return executor_.execute<command_code::set_led_config>( {static_cast<std::uint8_t>(mode), speed, static_cast<std::uint8_t>(color), cycles});
}

void_result
fingerprint_sensor::turn_led_on() noexcept
{
  spdlog::debug("turning led on...");
  return executor_.execute<command_code::turn_led_on>({});
}

void_result
fingerprint_sensor::turn_led_off() noexcept
{
  spdlog::debug("turning led off...");
  return executor_.execute<command_code::turn_led_off>({});
}

void_result
fingerprint_sensor::enable_auto_finger_detection() noexcept
{
  spdlog::debug("enabling auto finger detection...");
  return turn_led_on();
}

void_result
fingerprint_sensor::disable_auto_finger_detection() noexcept
{
  spdlog::debug("disabling auto finger detection...");
  return turn_led_off();
}

void_result
fingerprint_sensor::enter_standby_mode() noexcept
{
  spdlog::debug("entering standby mode...");
  return turn_led_off();
}

void_result
fingerprint_sensor::wake_from_standby() noexcept
{
  spdlog::debug("waking from standby...");
  return turn_led_on();
}

// --- security

void_result
fingerprint_sensor::set_device_password(std::uint32_t password) noexcept
{
  spdlog::debug("setting device password...");
  return executor_.execute<command_code::set_device_password>({password});
}

void_result
fingerprint_sensor::verify_device_password(std::uint32_t password) noexcept
{
  spdlog::debug("verifying device password...");
  return executor_.execute<command_code::verify_device_password>({password});
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
  spdlog::debug("capturing fingerprint image...");
  return executor_.execute<command_code::capture_image>({});
}

void_result
fingerprint_sensor::extract_features(std::uint8_t buffer_id)
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  spdlog::debug("extracting feature points of the fingerprint image...");
  return executor_.execute<command_code::extract_features>(std::array<std::uint8_t, 1>{buffer_id});
}

void_result
fingerprint_sensor::create_model() noexcept
{
  spdlog::debug("creating template model...");
  return executor_.execute<command_code::create_model>({});
}

void_result
fingerprint_sensor::store_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  spdlog::debug("storing template model...");
  return executor_.execute<command_code::store_model>({buffer_id, page_id});
}

void_result
fingerprint_sensor::load_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  spdlog::debug("loading template model...");
  return executor_.execute<command_code::load_model>({buffer_id, page_id});
}

void_result
fingerprint_sensor::upload_model(std::span<const std::uint8_t> buffer, std::uint8_t buffer_id) noexcept
{
  
  auto result = executor_.execute<command_code::upload_model>({buffer_id});
  if (!result) return make_error(result.error());
  spdlog::debug("uploading template model...");
  return executor_.send_data_packets(buffer);
}

void_result
fingerprint_sensor::download_model(std::span<std::uint8_t> buffer) noexcept
{
  // First send the download command
  spdlog::debug("downloading template model...");
  auto result = executor_.execute<command_code::download_model>({1}); // buffer_id = 1
  if (!result)
    return make_error(result.error());

  // Then receive data packets
  auto data_result = executor_.receive_data_packets();
  if (!data_result)
    return make_error(data_result.error());

  // Copy to output buffer
  std::size_t copy_size = std::min(buffer.size(), data_result->size());
  std::copy_n(data_result->begin(), copy_size, buffer.begin());

  spdlog::debug("template model downloaded successfully");
  return make_success();
}

// --- database ops

void_result
fingerprint_sensor::erase_model(std::uint16_t page_id, std::uint16_t count) noexcept
{
  spdlog::debug("erasing template model...");
  return executor_.execute<command_code::erase_model>({page_id, count});
}

void_result
fingerprint_sensor::clear_database() noexcept
{
  spdlog::debug("clearing database...");
  return executor_.execute<command_code::clear_database>({});
}

result<match_query_info>
fingerprint_sensor::match_model() noexcept
{
  spdlog::debug("matching template model...");
  return executor_.execute<command_code::match_model>({});
}

result<search_query_info>
fingerprint_sensor::search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  spdlog::debug("searching template model...");
  return executor_.execute<command_code::search_model>({buffer_id, page_id, count});
}

result<search_query_info>
fingerprint_sensor::fast_search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  spdlog::debug("fast searching template model...");
  return executor_.execute<command_code::fast_search_model>({buffer_id, page_id, count});
}

result<std::uint16_t>
fingerprint_sensor::model_count() noexcept
{
  spdlog::debug("view template model count...");
  return executor_.execute<command_code::count_model>({});
}

result<std::vector<std::uint8_t>>
fingerprint_sensor::read_index_table(std::span<std::uint8_t> data) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  spdlog::debug("reading index table...");
  auto result = executor_.execute<command_code::read_index_table>(std::array<std::uint8_t, 1>{0});
  if (!result)
    return make_error(result.error());

  // Copy to output buffer and return
  std::size_t copy_size = std::min(data.size(), result->size());
  std::copy_n(result->begin(), copy_size, data.begin());

  return make_success(std::vector<std::uint8_t>(result->begin(), result->end()));
}

} /* namespace carbio */
