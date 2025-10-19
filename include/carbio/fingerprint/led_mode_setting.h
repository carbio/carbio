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

#pragma once

#include <cstdint>

namespace carbio::fingerprint {
/*!
 * @brief The led mode setting.
 * Sets the led mode.
 */
enum class led_mode_setting : std::uint8_t {
  breathing = 0x01,
  flashing = 0x02,
  steady_on = 0x03,
  steady_off = 0x04,
  gradual_on = 0x05,
  gradual_off = 0x06,
};

/*!
 * @brief Convert the led mode setting to string representation.
 * @param setting The led mode setting.
 * @returns The string representation of led mode setting.
 */
[[nodiscard]] inline constexpr const char *
name(led_mode_setting setting) noexcept {
  switch (setting) {
  case led_mode_setting::breathing:
    return "breathing";
  case led_mode_setting::flashing:
    return "flashing";
  case led_mode_setting::steady_on:
    return "steady_on";
  case led_mode_setting::steady_off:
    return "steady_off";
  case led_mode_setting::gradual_on:
    return "gradual_on";
  case led_mode_setting::gradual_off:
    return "gradual_off";
  default:
    return "unknown";
  }
};

/*!
 * @brief Convert led mode setting to hexadecimal representation.
 * @param setting The led mode setting.
 * @returns The hexadecimal string representation of led mode setting.
 */
[[nodiscard]] inline constexpr const char *
hex_string(led_mode_setting setting) noexcept {
  switch (setting) {
  case led_mode_setting::breathing:
    return "0x01";
  case led_mode_setting::flashing:
    return "0x02";
  case led_mode_setting::steady_on:
    return "0x03";
  case led_mode_setting::steady_off:
    return "0x04";
  case led_mode_setting::gradual_on:
    return "0x05";
  case led_mode_setting::gradual_off:
    return "0x06";
  default:
    return "0xFF";
  }
};

/*!
 * @brief Convert led mode setting to hexadecimal representation.
 * @param setting The led mode setting.
 * @returns The hexadecimal string representation of led mode setting.
 */
[[nodiscard]] inline constexpr std::uint8_t
hex_value(led_mode_setting setting) noexcept {
  switch (setting) {
  case led_mode_setting::breathing:
    return 0x01;
  case led_mode_setting::flashing:
    return 0x02;
  case led_mode_setting::steady_on:
    return 0x03;
  case led_mode_setting::steady_off:
    return 0x04;
  case led_mode_setting::gradual_on:
    return 0x05;
  case led_mode_setting::gradual_off:
    return 0x06;
  default:
    return 0xFF;
  }
};

/*!
 * @brief Convert the led mode setting to human readable message.
 * @param setting The led mode setting.
 * @returns A human readable message that describes the led mode setting.
 */
[[nodiscard]] inline constexpr const char *
message(led_mode_setting setting) noexcept {
  switch (setting) {
  case led_mode_setting::breathing:
    return "breathing led mode";
  case led_mode_setting::flashing:
    return "flashing led mode";
  case led_mode_setting::steady_on:
    return "steady led mode (on)";
  case led_mode_setting::steady_off:
    return "steady led mode (off)";
  case led_mode_setting::gradual_on:
    return "gradual led mode (on)";
  case led_mode_setting::gradual_off:
    return "gradual led mode (off)";
  default:
    return "unknown led mode";
  }
};
} /* namespace carbio::fingerprint */
