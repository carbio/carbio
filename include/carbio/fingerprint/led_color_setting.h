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
 * @brief The led color setting.
 */
enum class led_color_setting : std::uint8_t {
  red = 0x01,    /*!< red led color */
  blue = 0x02,   /*!< blue led color */
  purple = 0x03, /*!< purple led color */
  green = 0x04,  /*!< green led color */
  yellow = 0x05, /*!< yellow led color */
  cyan = 0x06,   /*!< cyan led color */
  white = 0x07,  /*!< white led color */
};

/*!
 * @brief Convert the led color setting to string representation.
 * @param setting The led color setting.
 * @returns The string representation of led color setting.
 */
[[nodiscard]] inline constexpr const char *
name(led_color_setting setting) noexcept {
  switch (setting) {
  case led_color_setting::red:
    return "led_color_setting::red";
  case led_color_setting::blue:
    return "led_color_setting::blue";
  case led_color_setting::purple:
    return "led_color_setting::purple";
  case led_color_setting::green:
    return "led_color_setting::green";
  case led_color_setting::yellow:
    return "led_color_setting::yellow";
  case led_color_setting::cyan:
    return "led_color_setting::cyan";
  case led_color_setting::white:
    return "led_color_setting::white";
  default:
    return "<unspecified enum value>";
  }
};

/*!
 * @brief Convert a given led color setting to string representation of
 * corresponding device register.
 * @param setting The led color setting.
 * @returns The string representation of device register address in
 * hexadecimals.
 */
[[nodiscard]] inline constexpr const char *
hex_string(led_color_setting setting) noexcept {
  switch (setting) {
  case led_color_setting::red:
    return "0x01";
  case led_color_setting::blue:
    return "0x02";
  case led_color_setting::purple:
    return "0x03";
  case led_color_setting::green:
    return "0x04";
  case led_color_setting::yellow:
    return "0x05";
  case led_color_setting::cyan:
    return "0x06";
  case led_color_setting::white:
    return "0x07";
  default:
    return "0xFF";
  }
};

/*!
 * @brief Convert a given led color setting to string representation of
 * corresponding device register.
 * @param setting The led color setting.
 * @returns The string representation of device register address in
 * hexadecimals.
 */
[[nodiscard]] inline constexpr std::uint8_t
hex_value(led_color_setting setting) noexcept {
  switch (setting) {
  case led_color_setting::red:
    return 0x01;
  case led_color_setting::blue:
    return 0x02;
  case led_color_setting::purple:
    return 0x03;
  case led_color_setting::green:
    return 0x04;
  case led_color_setting::yellow:
    return 0x05;
  case led_color_setting::cyan:
    return 0x06;
  case led_color_setting::white:
    return 0x07;
  default:
    return 0xFF;
  }
};

/*!
 * @brief Convert a given led color setting to human readable message.
 * @param value The led color setting.
 * @returns A human readable string representation of the given led color
 * setting.
 */
[[nodiscard]] inline constexpr const char *
message(led_color_setting setting) noexcept {
  switch (setting) {
  case led_color_setting::red:
    return "red led color";
  case led_color_setting::blue:
    return "blue led color";
  case led_color_setting::purple:
    return "purple led color";
  case led_color_setting::green:
    return "green led color";
  case led_color_setting::yellow:
    return "yellow led color";
  case led_color_setting::cyan:
    return "cyan led color";
  case led_color_setting::white:
    return "white led color";
  default:
    return "<unspecified enum value>";
  }
};
} // namespace carbio::fingerprint
