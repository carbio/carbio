/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
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

#ifndef CARBIO_DEVICE_SETTING_INDEX_H
#define CARBIO_DEVICE_SETTING_INDEX_H

#include <cstdint>
#include <optional>
#include <string_view>

namespace carbio
{
/*!
 * @brief The device setting index.
 * Selects the device setting option in system register.
 */
enum class device_setting_index : std::uint8_t
{
  baud_rate_setting      = 4, /*!< The baud rate setting */
  security_level_setting = 5, /*!< The security level setting */
  packet_length_setting  = 6, /*!< The packet length setting */
};

/*!
 * @brief Convert the device setting index to string representation.
 * @param index The device setting index.
 * @returns The string representation of device setting index.
 */
[[nodiscard]] inline constexpr std::string_view get_name(device_setting_index index) noexcept
{
  using namespace std::literals;
  switch (index)
  {
    case device_setting_index::baud_rate_setting:
      return "baud_rate_setting"sv;
    case device_setting_index::security_level_setting:
      return "security_level_setting"sv;
    case device_setting_index::packet_length_setting:
      return "packet_length_setting"sv;
    default:
      return "unknown_setting"sv;
  }
};

/*!
 * @brief Convert the device setting index to hexadecimal representation.
 * @param index The device setting index.
 * @returns The hexadecimal representation of device setting index.
 */
[[nodiscard]] inline constexpr std::string_view get_hex(device_setting_index index) noexcept
{
  using namespace std::literals;
  switch (index)
  {
    case device_setting_index::baud_rate_setting:
      return "0x04"sv;
    case device_setting_index::security_level_setting:
      return "0x05"sv;
    case device_setting_index::packet_length_setting:
      return "0x06"sv;
    default:
      return "0x00"sv;
  }
};

/*!
 * @brief Convert the device setting index to human readable message.
 * @param index The device setting index.
 * @returns The human readable string that describes the device setting index.
 */
[[nodiscard]] inline constexpr std::string_view get_message(device_setting_index index) noexcept
{
  using namespace std::literals;
  switch (index)
  {
    case device_setting_index::baud_rate_setting:
      return "baud rate setting"sv;
    case device_setting_index::security_level_setting:
      return "security level setting"sv;
    case device_setting_index::packet_length_setting:
      return "packet length setting"sv;
    default:
      return "unknown setting"sv;
  }
};
} /* namespace carbio */

#endif /* CARBIO_DEVICE_SETTING_INDEX_H */