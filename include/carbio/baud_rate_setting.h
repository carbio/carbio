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

#ifndef CARBIO_BAUD_RATE_SETTING_H
#define CARBIO_BAUD_RATE_SETTING_H

#include <cstdint>
#include <string_view>

namespace carbio
{
/*!
 * @brief The baud rate setting.
 * Sets the brandwidth speed.
 */
enum class baud_rate_setting : std::uint8_t
{
  _9600   = 0x01,   /*!< 9600 bps */
  _19200  = 0x02,   /*!< 19200 bps */
  _28800  = 0x03,   /*!< 28800 bps */
  _38400  = 0x04,   /*!< 38400 bps */
  _48000  = 0x05,   /*!< 48000 bps */
  _57600  = 0x06,   /*!< 57600 bps */
  _67200  = 0x07,   /*!< 67200 bps */
  _76800  = 0x08,   /*!< 76800 bps */
  _86400  = 0x09,   /*!< 86400 bps */
  _96000  = 0x0A,   /*!< 96000 bps */
  _105600 = 0x0B,   /*!< 105600 bps */
  _115200 = 0x0C,   /*!< 115200 bps */
};

/*!
 * @brief Convert the baud rate setting to string representation.
 * @param setting The baud rate setting.
 * @returns The string representation of baud rate setting.
 */
[[nodiscard]] inline constexpr std::string_view get_name(baud_rate_setting setting) noexcept
{
  switch (setting)
  {
    case baud_rate_setting::_9600:
      return "9600";
    case baud_rate_setting::_19200:
      return "19200";
    case baud_rate_setting::_28800:
      return "28800";
    case baud_rate_setting::_38400:
      return "38400";
    case baud_rate_setting::_48000:
      return "48000";
    case baud_rate_setting::_57600:
      return "57600";
    case baud_rate_setting::_67200:
      return "67200";
    case baud_rate_setting::_76800:
      return "76800";
    case baud_rate_setting::_86400:
      return "86400";
    case baud_rate_setting::_96000:
      return "96000";
    case baud_rate_setting::_105600:
      return "105600";
    case baud_rate_setting::_115200:
      return "115200";
  }
};

/*!
 * @brief Convert the baud rate setting to hexadecimal representation.
 * @param setting The baud rate setting.
 * @returns The hexadecimal representation of baud rate setting.
 */
[[nodiscard]] inline constexpr std::string_view get_hex(baud_rate_setting setting) noexcept
{
  switch (setting)
  {
    case baud_rate_setting::_9600:
      return "0x01";
    case baud_rate_setting::_19200:
      return "0x02";
    case baud_rate_setting::_28800:
      return "0x03";
    case baud_rate_setting::_38400:
      return "0x04";
    case baud_rate_setting::_48000:
      return "0x05";
    case baud_rate_setting::_57600:
      return "0x06";
    case baud_rate_setting::_67200:
      return "0x07";
    case baud_rate_setting::_76800:
      return "0x08";
    case baud_rate_setting::_86400:
      return "0x09";
    case baud_rate_setting::_96000:
      return "0x0A";
    case baud_rate_setting::_105600:
      return "0x0B";
    case baud_rate_setting::_115200:
      return "0x0C";
  }
};

/*!
 * @brief Convert the baud rate setting to human readable message.
 * @param setting The baud rate setting.
 * @returns The human readable string that describes the baud rate setting.
 */
[[nodiscard]] inline constexpr std::string_view get_message(baud_rate_setting setting) noexcept
{
  switch (setting)
  {
    case baud_rate_setting::_9600:
      return "9600 bps";
    case baud_rate_setting::_19200:
      return "19200 bps";
    case baud_rate_setting::_28800:
      return "28800 bps";
    case baud_rate_setting::_38400:
      return "38400 bps";
    case baud_rate_setting::_48000:
      return "48000 bps";
    case baud_rate_setting::_57600:
      return "57600 bps";
    case baud_rate_setting::_67200:
      return "67200 bps";
    case baud_rate_setting::_76800:
      return "76800 bps";
    case baud_rate_setting::_86400:
      return "86400 bps";
    case baud_rate_setting::_96000:
      return "96000 bps";
    case baud_rate_setting::_105600:
      return "105600 bps";
    case baud_rate_setting::_115200:
      return "115200 bps";
  }
};
} // namespace carbio

#endif // CARBIO_BAUD_RATE_SETTING_H
