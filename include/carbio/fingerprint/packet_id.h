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
 * @brief packet identifier
 */
enum class packet_id : std::uint8_t {
  command = 0x01,     /*!< command */
  data = 0x02,        /*!< data */
  acknowledge = 0x07, /*!< acknowledge */
  end_data = 0x08,    /*!< end of data */
};

/*!
 * @brief Convert packet identifier to string representation.
 * @param type The packet identifier.
 * @returns The string representation of packet identifier.
 */
[[nodiscard]] inline constexpr std::string_view name(packet_id type) noexcept {
  switch (type) {
  case packet_id::command:
    return "command";
  case packet_id::data:
    return "data";
  case packet_id::acknowledge:
    return "acknowledge";
  case packet_id::end_data:
    return "end_data";
  default:
    return "<unspecified enum value>";
  }
};

/*!
 * @brief Convert packet identifier to hexadecimal representation.
 * @param type The packet identifier.
 * @returns The hexadecimal representation of packet identifier.
 */
[[nodiscard]] inline constexpr const char *hex_string(packet_id type) noexcept {
  switch (type) {
  case packet_id::command:
    return "0x01";
  case packet_id::data:
    return "0x02";
  case packet_id::acknowledge:
    return "0x07";
  case packet_id::end_data:
    return "0x08";
  default:
    return "0xFF";
  }
};

/*!
 * @brief Convert the packet identifier to human readable message.
 * @param type The packet identifier.
 * @returns A human readable message that describes the packet identifier.
 */
[[nodiscard]] inline constexpr const char *message(packet_id type) noexcept {
  switch (type) {
  case packet_id::command:
    return "command package type";
  case packet_id::data:
    return "data package type";
  case packet_id::acknowledge:
    return "acknowledge package type";
  case packet_id::end_data:
    return "end of data package type";
  default:
    return "<unspecified enum value>";
  }
};
} // namespace carbio::fingerprint
