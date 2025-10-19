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
 * @brief The packet data length setting.
 * Switches internal packet setting in sensor.
 */
enum class packet_data_length_setting : std::uint8_t {
  _32 = 0x00,  /*!< 32-byte packet data length */
  _64 = 0x01,  /*!< 64-byte packet data length */
  _128 = 0x02, /*!< 128-byte packet data length */
  _256 = 0x03, /*!< 256-byte packet data length */
};

/*!
 * @brief Convert the package data length setting to underlying string
 * representation.
 * @param setting The package data length setting.
 * @returns The underlying string representation of package data length setting.
 */
[[nodiscard]] inline constexpr const char *
name(packet_data_length_setting setting) noexcept {
  switch (setting) {
  case packet_data_length_setting::_32:
    return "packet_data_length_setting::_32";
  case packet_data_length_setting::_64:
    return "packet_data_length_setting::_64";
  case packet_data_length_setting::_128:
    return "packet_data_length_setting::_128";
  case packet_data_length_setting::_256:
    return "packet_data_length_setting::_256";
  default:
    return "<unspecified enum value>";
  }
};

/*!
 * @brief Convert package data length setting to hexadecimal string value.
 * @param setting The package data length setting.
 * @returns The hexadecimal representation of package data length setting.
 */
[[nodiscard]] inline constexpr const char *
hex_string(packet_data_length_setting setting) noexcept {
  switch (setting) {
  case packet_data_length_setting::_32:
    return "0x00";
  case packet_data_length_setting::_64:
    return "0x01";
  case packet_data_length_setting::_128:
    return "0x02";
  case packet_data_length_setting::_256:
    return "0x03";
  default:
    return "<unspecified enum value>";
  }
};

/*!
 * @brief Convert package data length setting to hexadecimal integer value.
 * @param setting The package data length setting.
 * @returns The hexadecimal representation of package data length setting.
 */
[[nodiscard]] inline constexpr std::uint8_t
hex_value(packet_data_length_setting setting) noexcept {
  switch (setting) {
  case packet_data_length_setting::_32:
    return 0x0;
  case packet_data_length_setting::_64:
    return 0x1;
  case packet_data_length_setting::_128:
    return 0x2;
  case packet_data_length_setting::_256:
    return 0x3;
  default:
    return 0xFF;
  }
};

/*!
 * @brief Convert packet data length setting to human-readable description
 * message.
 * @param setting The packet data length setting.
 * @returns A human-readable message string that describes the packet data
 * length setting.
 */
[[nodiscard]] inline constexpr const char *
message(packet_data_length_setting setting) noexcept {
  switch (setting) {
  case packet_data_length_setting::_32:
    return "32-bytes data packets";
  case packet_data_length_setting::_64:
    return "64-bytes data packets";
  case packet_data_length_setting::_128:
    return "128-bytes data packets";
  case packet_data_length_setting::_256:
    return "256-bytes data packets";
  default:
    return "<unspecified enum value>";
  }
};
} // namespace carbio::fingerprint
