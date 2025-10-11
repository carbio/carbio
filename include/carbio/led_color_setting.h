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

#ifndef CARBIO_LED_COLOR_SETTING_H
#define CARBIO_LED_COLOR_SETTING_H

#include <cstdint>
#include <string_view>

namespace carbio
{
/*!
 * @brief The led color setting.
 */
enum class led_color_setting : std::uint8_t
{
  red    = 0x01, /*!< red led color */
  blue   = 0x02, /*!< blue led color */
  purple = 0x03, /*!< purple led color */
  green  = 0x04, /*!< green led color */
  yellow = 0x05, /*!< yellow led color */
  cyan   = 0x06, /*!< cyan led color */
  white  = 0x07, /*!< white led color */
};

/*!
 * @brief Convert the led color setting to string representation.
 * @param setting The led color setting.
 * @returns The string representation of led color setting.
 */
[[nodiscard]] inline constexpr std::string_view get_name(led_color_setting setting) noexcept
{
  using namespace std::literals;
  switch (setting)
  {
    case led_color_setting::red:
      return "red"sv;
    case led_color_setting::blue:
      return "blue"sv;
    case led_color_setting::purple:
      return "purple"sv;
    case led_color_setting::green:
      return "green"sv;
    case led_color_setting::yellow:
      return "yellow"sv;
    case led_color_setting::cyan:
      return "cyan"sv;
    case led_color_setting::white:
      return "white"sv;
    default:
      return "unknown"sv;
  }
};

/*!
 * @brief Convert a given led color setting to string representation of corresponding device register.
 * @param setting The led color setting.
 * @returns The string representation of device register address in hexadecimals.
 */
[[nodiscard]] inline constexpr std::string_view get_address(led_color_setting setting) noexcept
{
  using namespace std::literals;
  switch (setting)
  {
    case led_color_setting::red:
      return "0x01"sv;
    case led_color_setting::blue:
      return "0x02"sv;
    case led_color_setting::purple:
      return "0x03"sv;
    case led_color_setting::green:
      return "0x04"sv;
    case led_color_setting::yellow:
      return "0x05"sv;
    case led_color_setting::cyan:
      return "0x06"sv;
    case led_color_setting::white:
      return "0x07"sv;
    default:
      return "0x00"sv;
  }
};

/*!
 * @brief Convert a given led color setting to human readable message.
 * @param value The led color setting.
 * @returns A human readable string representation of the given led color setting.
 */
[[nodiscard]] inline constexpr std::string_view get_message(led_color_setting value) noexcept
{
  using namespace std::literals;
  switch (value)
  {
    case led_color_setting::red:
      return "red led color"sv;
    case led_color_setting::blue:
      return "blue led color"sv;
    case led_color_setting::purple:
      return "purple led color"sv;
    case led_color_setting::green:
      return "green led color"sv;
    case led_color_setting::yellow:
      return "yellow led color"sv;
    case led_color_setting::cyan:
      return "cyan led color"sv;
    case led_color_setting::white:
      return "white led color"sv;
    default:
      return "unknown led color"sv;
  }
};
} /* namespace carbio */

#endif /* CARBIO_LED_COLOR_SETTING_H */
