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
 * @brief The security level setting.
 * Sets the security level of the fingerprint sensor.
 */
enum class security_level_setting : std::uint8_t {
  lowest = 0x01, /*!< lowest security level with fast matching algorithm at the
                    cost of reliability. */
  low = 0x02, /*!< low security level with slightly faster matching algorithm at
                 the cost of reliability. */
  balanced = 0x03, /*!< balanced security level with reasonably reliable
                      matching algorithm with good speed. */
  high = 0x04,     /*!< high security level with slightly more reliable matching
                      algorithm at the cost of speed. */
  highest = 0x05,  /*!< highest security level with very strict matching
                      algorithm at the cost of speed. */
};

/*!
 * @brief Converts the security level setting to string representation.
 * @param setting The security level setting.
 * @returns A string representation of security level setting.
 */
[[nodiscard]] inline constexpr const char *
name(security_level_setting setting) noexcept {
  switch (setting) {
  case security_level_setting::lowest:
    return "lowest";
  case security_level_setting::low:
    return "low";
  case security_level_setting::balanced:
    return "balanced";
  case security_level_setting::high:
    return "high";
  case security_level_setting::highest:
    return "highest";
  default:
    return "unknown";
  }
};

/*!
 * @brief Converts the security level setting to hexadecimal string
 * representation.
 * @param setting The security level setting.
 * @returns A hexadecimal string representation of security level setting.
 */
[[nodiscard]] inline constexpr const char *
hex_string(security_level_setting setting) noexcept {
  switch (setting) {
  case security_level_setting::lowest:
    return "0x01";
  case security_level_setting::low:
    return "0x02";
  case security_level_setting::balanced:
    return "0x03";
  case security_level_setting::high:
    return "0x04";
  case security_level_setting::highest:
    return "0x05";
  default:
    return "0xFF";
  }
};

/*!
 * @brief Converts the security level setting to hexadecimal string
 * representation.
 * @param setting The security level setting.
 * @returns A hexadecimal string representation of security level setting.
 */
[[nodiscard]] inline constexpr std::uint8_t
hex_value(security_level_setting setting) noexcept {
  switch (setting) {
  case security_level_setting::lowest:
    return 0x01;
  case security_level_setting::low:
    return 0x02;
  case security_level_setting::balanced:
    return 0x03;
  case security_level_setting::high:
    return 0x04;
  case security_level_setting::highest:
    return 0x05;
  default:
    return 0xFF;
  }
};

/*!
 * @brief Converts the security level setting to human readable message.
 * @param setting The security level setting.
 * @returns A human readable string that describes the security level setting.
 */
[[nodiscard]] inline constexpr const char *
message(security_level_setting setting) noexcept {
  switch (setting) {
  case security_level_setting::lowest:
    return "fastest matching algorithm at the cost of reliability";
  case security_level_setting::low:
    return "slightly faster matching algorithm at the cost of reliability";
  case security_level_setting::balanced:
    return "reasonably fast and reliable matching algorithm";
  case security_level_setting::high:
    return "slightly more reliable matching algorithm at the cost of speed";
  case security_level_setting::highest:
    return "most reliable matching algorithm at the cost of speed";
  default:
    return "unknown security level";
  }
};
} // namespace carbio::fingerprint
