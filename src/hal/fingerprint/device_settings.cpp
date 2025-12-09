/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#include "fingerprint/device_settings.h"

#include "fingerprint/sensor_types.h"

#include <string_view>

#if defined(__cpp_lib_format)
#  include <format>
#else
#  include <fmt/format.h>
#endif

namespace
{
using namespace std::string_view_literals;

[[nodiscard]] constexpr std::string_view to_string(carbio::baud_rate_setting setting) noexcept
{
  switch (setting)
  {
  case carbio::baud_rate_setting::_9600:
    return "9600"sv;
  case carbio::baud_rate_setting::_19200:
    return "19200"sv;
  case carbio::baud_rate_setting::_28800:
    return "28800"sv;
  case carbio::baud_rate_setting::_38400:
    return "38400"sv;
  case carbio::baud_rate_setting::_48000:
    return "48000"sv;
  case carbio::baud_rate_setting::_57600:
    return "57600"sv;
  case carbio::baud_rate_setting::_67200:
    return "67200"sv;
  case carbio::baud_rate_setting::_76800:
    return "76800"sv;
  case carbio::baud_rate_setting::_86400:
    return "86400"sv;
  case carbio::baud_rate_setting::_96000:
    return "96000"sv;
  case carbio::baud_rate_setting::_105600:
    return "105600"sv;
  case carbio::baud_rate_setting::_115200:
    return "115200"sv;
  }
  return "unknown"sv;
}

[[nodiscard]] constexpr std::string_view to_string(carbio::packet_length_setting setting) noexcept
{
  switch (setting)
  {
  case carbio::packet_length_setting::_32:
    return "32"sv;
  case carbio::packet_length_setting::_64:
    return "64"sv;
  case carbio::packet_length_setting::_128:
    return "128"sv;
  case carbio::packet_length_setting::_256:
    return "256"sv;
  }
  return "unknown"sv;
}

[[nodiscard]] constexpr std::string_view to_string(carbio::security_level_setting setting) noexcept
{
  switch (setting)
  {
  case carbio::security_level_setting::lowest:
    return "lowest"sv;
  case carbio::security_level_setting::low:
    return "low"sv;
  case carbio::security_level_setting::balanced:
    return "balanced"sv;
  case carbio::security_level_setting::high:
    return "high"sv;
  case carbio::security_level_setting::highest:
    return "highest"sv;
  }
  return "unknown"sv;
}

[[nodiscard]] constexpr std::string_view to_string(carbio::status_code status) noexcept
{
  switch (status)
  {
  case carbio::status_code::success:
    return "success"sv;
  case carbio::status_code::frame_error:
    return "frame_error"sv;
  case carbio::status_code::no_finger:
    return "no_finger"sv;
  case carbio::status_code::image_capture_error:
    return "image_capture_error"sv;
  case carbio::status_code::image_too_faint:
    return "image_too_faint"sv;
  case carbio::status_code::image_too_blurry:
    return "image_too_blurry"sv;
  case carbio::status_code::image_too_distorted:
    return "image_too_distorted"sv;
  case carbio::status_code::image_too_few_features:
    return "image_too_few_features"sv;
  case carbio::status_code::no_match:
    return "no_match"sv;
  case carbio::status_code::not_found:
    return "not_found"sv;
  case carbio::status_code::enrollment_mismatch:
    return "enrollment_mismatch"sv;
  case carbio::status_code::index_out_of_range:
    return "index_out_of_range"sv;
  case carbio::status_code::database_access_error:
    return "database_access_error"sv;
  case carbio::status_code::feature_upload_failed:
    return "feature_upload_failed"sv;
  case carbio::status_code::no_frame:
    return "no_frame"sv;
  case carbio::status_code::image_upload_failed:
    return "image_upload_failed"sv;
  case carbio::status_code::erase_failed:
    return "erase_failed"sv;
  case carbio::status_code::database_clear_failed:
    return "database_clear_failed"sv;
  case carbio::status_code::cannot_enter_low_power_mode:
    return "cannot_enter_low_power_mode"sv;
  case carbio::status_code::permission_denied:
    return "permission_denied"sv;
  case carbio::status_code::invalid_image_format:
    return "invalid_image_format"sv;
  case carbio::status_code::device_busy:
    return "device_busy"sv;
  case carbio::status_code::flash_write_error:
    return "flash_write_error"sv;
  case carbio::status_code::unknown_error:
    return "unknown_error"sv;
  case carbio::status_code::illegal_device_register:
    return "illegal_device_register"sv;
  case carbio::status_code::invalid_device_configuration:
    return "invalid_device_configuration"sv;
  case carbio::status_code::communication_error:
    return "communication_error"sv;
  case carbio::status_code::database_full:
    return "database_full"sv;
  case carbio::status_code::illegal_device_address:
    return "illegal_device_address"sv;
  case carbio::status_code::device_authorization_required:
    return "device_authorization_required"sv;
  case carbio::status_code::hardware_fault:
    return "hardware_fault"sv;
  case carbio::status_code::unsupported_command:
    return "unsupported_command"sv;
  case carbio::status_code::bad_packet:
    return "bad_packet"sv;
  case carbio::status_code::timeout:
    return "timeout"sv;
  }
  return "unknown_error"sv;
}
} // namespace

namespace carbio
{
std::string to_json(const device_settings& s) noexcept
{
#if defined(__cpp_lib_format)
  return std::format("{{ \"status\": \"{}\","
                     " \"id\": {},"
                     " \"capacity\": {},"
                     " \"security_level\": \"{}\","
                     " \"address\": 0x{:08X},"
                     " \"length\": \"{}\","
                     " \"baudrate\": \"{}\" }}",
                     to_string(static_cast<status_code>(s.status)), s.id, s.capacity, to_string(static_cast<security_level_setting>(s.security_level)), s.address, to_string(static_cast<packet_length_setting>(s.length)),
                     to_string(static_cast<baud_rate_setting>(s.baudrate)));
#else
  return fmt::format(FMT_STRING("{{ \"status\": \"{}\","
                                " \"id\": {},"
                                " \"capacity\": {},"
                                " \"security_level\": \"{}\","
                                " \"address\": 0x{:08X},"
                                " \"length\": \"{}\","
                                " \"baudrate\": \"{}\" }}"),
                     to_string(static_cast<status_code>(s.status)), s.id, s.capacity, to_string(static_cast<security_level_setting>(s.security_level)), s.address, to_string(static_cast<packet_length_setting>(s.length)),
                     to_string(static_cast<baud_rate_setting>(s.baudrate)));
#endif
}
} // namespace carbio