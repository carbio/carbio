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

#ifndef CARBIO_DATA_LENGTH_SETTING_H
#define CARBIO_DATA_LENGTH_SETTING_H

#include <cstdint>
#include <string_view>

namespace carbio
{
/*!
 * @brief The data length setting.
 */
enum class data_length_setting : std::uint8_t
{
  _32     = 0x00, /*!< 32-byte data length */
  _64     = 0x01, /*!< 64-byte data length */
  _128    = 0x02, /*!< 128-byte data length */
  _256    = 0x03, /*!< 256-byte data length */
};

/*!
 * @brief Convert the data length setting to string representation
 * @param setting The data length setting.
 * @returns The string representation of data length setting.
 */
[[nodiscard]] inline constexpr std::string_view get_name(data_length_setting setting) noexcept
{
  switch (setting)
  {
    case data_length_setting::_32:
      return "32";
    case data_length_setting::_64:
      return "64";
    case data_length_setting::_128:
      return "128";
    case data_length_setting::_256:
      return "256";
  }
};

/*!
 * @brief Convert data length setting to machine code.
 * @param setting The data length setting.
 * @returns The hexadecimal representation of data length setting.
 */
[[nodiscard]] inline constexpr std::string_view get_hex(data_length_setting setting) noexcept
{
  switch (setting)
  {
    case data_length_setting::_32:
      return "0x01";
    case data_length_setting::_64:
      return "0x02";
    case data_length_setting::_128:
      return "0x03";
    case data_length_setting::_256:
      return "0x04";
  }
};

/*!
 * @brief Convert data length setting to human readable message.
 * @param setting The data length setting.
 * @returns The human readable string that describes the data length setting.
 */
[[nodiscard]] inline constexpr std::string_view get_message(data_length_setting setting) noexcept
{
  switch (setting)
  {
    case data_length_setting::_32:
      return "32-byte data length";
    case data_length_setting::_64:
      return "64-byte data length";
    case data_length_setting::_128:
      return "128-byte data length";
    case data_length_setting::_256:
      return "256-byte data length";
  }
};
} // namespace carbio

#endif // CARBIO_DATA_LENGTH_SETTING_H
