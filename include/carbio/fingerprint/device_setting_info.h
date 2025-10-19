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
#include <string>

namespace carbio::fingerprint {
/*!
 * @brief Device setting information
 */
struct device_setting_info {
  std::uint16_t status{0};           /*!< current status */
  std::uint16_t id{0};               /*!< sensor type */
  std::uint16_t capacity{127};       /*!< database capacity */
  std::uint16_t security_level{0};   /*!< current security level setting */
  std::uint32_t address{0xFFFFFFFF}; /*!< current device address setting */
  std::uint16_t length{128};         /*!< current packet length setting  */
  std::uint16_t baudrate{57600};     /*!< current baud rate setting */
};

/*!
 * @brief Retrieves the default device configuration.
 * @returns The default device configuration.
 */
extern device_setting_info get_default_device_setting_info() noexcept;

/*!
 * @brief Convert device setting information to JSON string
 * @param info The device setting information
 * @returns JSON string format
 */
extern std::string to_json(const device_setting_info &info) noexcept;
} // namespace carbio::fingerprint
