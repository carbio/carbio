#pragma once

#include <cstdint>
#include <string_view>

namespace carbio
{

enum class packet_id : std::uint8_t
{
  command = 0x01,
  data = 0x02,
  acknowledge = 0x07,
  end_data = 0x08,
};

enum class baud_rate_setting : std::uint8_t
{
  _9600 = 0x01,
  _19200 = 0x02,
  _28800 = 0x03,
  _38400 = 0x04,
  _48000 = 0x05,
  _57600 = 0x06,
  _67200 = 0x07,
  _76800 = 0x08,
  _86400 = 0x09,
  _96000 = 0x0A,
  _105600 = 0x0B,
  _115200 = 0x0C,
};

enum class packet_length_setting : std::uint8_t
{
  _32 = 0x00,
  _64 = 0x01,
  _128 = 0x02,
  _256 = 0x03,
};

enum class security_level_setting : std::uint8_t
{
  lowest = 0x01,
  low = 0x02,
  balanced = 0x03,
  high = 0x04,
  highest = 0x05,
};

enum class led_color_setting : std::uint8_t
{
  red = 0x01,
  blue = 0x02,
  purple = 0x03,
  green = 0x04,
  yellow = 0x05,
  cyan = 0x06,
  white = 0x07,
};

enum class led_mode_setting : std::uint8_t
{
  breathing = 0x01,
  flashing = 0x02,
  steady_on = 0x03,
  steady_off = 0x04,
  gradual_on = 0x05,
  gradual_off = 0x06,
};

enum class device_setting_index : std::uint8_t
{
  baud_rate_setting = 4,
  security_level_setting = 5,
  packet_length_setting = 6,
};

enum class status_code : std::uint8_t
{
  success = 0x00,
  frame_error = 0x01,
  no_finger = 0x02,
  image_capture_error = 0x03,
  image_too_faint = 0x04,
  image_too_blurry = 0x05,
  image_too_distorted = 0x06,
  image_too_few_features = 0x07,
  no_match = 0x08,
  not_found = 0x09,
  enrollment_mismatch = 0x0A,
  index_out_of_range = 0x0B,
  database_access_error = 0x0C,
  feature_upload_failed = 0x0D,
  no_frame = 0x0E,
  image_upload_failed = 0x0F,
  erase_failed = 0x10,
  database_clear_failed = 0x11,
  cannot_enter_low_power_mode = 0x12,
  permission_denied = 0x13,
  invalid_image_format = 0x15,
  device_busy = 0x17,
  flash_write_error = 0x18,
  unknown_error = 0x19,
  illegal_device_register = 0x1A,
  invalid_device_configuration = 0x1B,
  communication_error = 0x1D,
  database_full = 0x1F,
  illegal_device_address = 0x20,
  device_authorization_required = 0x21,
  hardware_fault = 0x29,
  unsupported_command = 0xFC,
  bad_packet = 0xFE,
  timeout = 0xFF,
};

[[nodiscard]] constexpr std::string_view message(status_code status) noexcept
{
  using namespace std::string_view_literals;
  switch (status)
  {
  case status_code::success:
    return "operation succeeded"sv;
  case status_code::frame_error:
    return "packet frame receive error"sv;
  case status_code::no_finger:
    return "no finger detected"sv;
  case status_code::image_capture_error:
    return "image capture error"sv;
  case status_code::image_too_faint:
    return "image too faint"sv;
  case status_code::image_too_blurry:
    return "image too blurry"sv;
  case status_code::image_too_distorted:
    return "image too distorted"sv;
  case status_code::image_too_few_features:
    return "image contains too few feature points"sv;
  case status_code::no_match:
    return "fingerprint not match"sv;
  case status_code::not_found:
    return "fingerprint not found"sv;
  case status_code::enrollment_mismatch:
    return "enrollment mismatch"sv;
  case status_code::index_out_of_range:
    return "index is out of range"sv;
  case status_code::database_access_error:
    return "storage access error"sv;
  case status_code::feature_upload_failed:
    return "feature upload failed"sv;
  case status_code::no_frame:
    return "no packet frame"sv;
  case status_code::image_upload_failed:
    return "image upload failed"sv;
  case status_code::erase_failed:
    return "erase failed"sv;
  case status_code::database_clear_failed:
    return "failed deleting database"sv;
  case status_code::cannot_enter_low_power_mode:
    return "cannot enter low power mode"sv;
  case status_code::permission_denied:
    return "permission denied due to incorrect device password"sv;
  case status_code::device_busy:
    return "device busy - sensor internal operation in progress"sv;
  case status_code::invalid_image_format:
    return "invalid image format"sv;
  case status_code::flash_write_error:
    return "flash write failed"sv;
  case status_code::unknown_error:
    return "unknown error"sv;
  case status_code::illegal_device_register:
    return "illegal access to device register"sv;
  case status_code::invalid_device_configuration:
    return "invalid device configuration"sv;
  case status_code::communication_error:
    return "communication interface inaccessible"sv;
  case status_code::database_full:
    return "database is full"sv;
  case status_code::illegal_device_address:
    return "illegal access to device address"sv;
  case status_code::device_authorization_required:
    return "device requires password authorization"sv;
  case status_code::hardware_fault:
    return "operation failed due hardware failure"sv;
  case status_code::unsupported_command:
    return "operation not supported"sv;
  case status_code::bad_packet:
    return "bad packet"sv;
  case status_code::timeout:
    return "operation timed out"sv;
  }
  return "unknown error"sv;
}

} // namespace carbio
