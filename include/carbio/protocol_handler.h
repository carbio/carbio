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

#ifndef CARBIO_PROTOCOL_HANDLER_H
#define CARBIO_PROTOCOL_HANDLER_H

#include "carbio/command_code.h"
#include "carbio/packet_id.h"
#include "carbio/result.h"

#include <cstdint>
#include <span>
#include <vector>

namespace carbio
{
class protocol_handler
{
  std::uint32_t address_{0xFFFFFFFF};
  std::uint16_t length_{128};

public:
  protocol_handler() noexcept = default;
  explicit protocol_handler(std::uint32_t new_address) noexcept : address_(new_address)
  {
  }
  protocol_handler(protocol_handler &&other) noexcept            = default;
  protocol_handler &operator=(protocol_handler &&other) noexcept = default;
  protocol_handler(const protocol_handler &other)                = delete;
  protocol_handler &operator=(const protocol_handler &other)     = delete;

  /*!
   * @brief Set device address.
   * @param new_address The new device address.
   */
  inline constexpr void
  set_address(std::uint32_t new_address) noexcept
  {
    address_ = new_address;
  }

  /*!
   * @brief Get current device address.
   * @returns Device address
   */
  [[nodiscard]] inline constexpr std::uint32_t
  get_address() const noexcept
  {
    return address_;
  }

  /*!
   * @brief Set packet length.
   * @param new_length Packet length (either 32, 64, 128, or 256 bytes)
   */
  inline constexpr void
  set_packet_length(std::uint16_t new_length) noexcept
  {
    length_ = new_length;
  }

  /*!
   * @brief Get current packet length.
   * @returns Packet length in bytes
   */
  [[nodiscard]] inline constexpr std::uint16_t
  get_packet_length() const noexcept
  {
    return length_;
  }

  /*!
   * @brief Construct a command packet.
   * @param code Command code.
   * @param data Command parameters.
   * @returns Response from device.
   */
  result<std::vector<std::uint8_t>> construct_command_packet(command_code code, std::span<const std::uint8_t> data) const noexcept;

  /*!
   * @brief Parse acknowledge packet.
   * @param data 
   * @returns Response from device.
   */
  result<std::vector<std::uint8_t>> parse_acknowledge_packet(std::span<const std::uint8_t> data) const noexcept;

  /*!
   * @brief Construct a data packet.
   * @param data Byte stream. 
   * @returns Response from device.
   */
  result<std::vector<std::vector<std::uint8_t>>> construct_data_packet(std::span<const std::uint8_t> data) const noexcept;

  /*!
   * @brief Parse a data packet.
   * @param data Byte stream.
   * @returns Response from device.
   */
  result<std::vector<std::uint8_t>> parse_data_packet(std::span<const std::span<std::uint8_t>> data) const noexcept;
};
} /* namespace carbio */

#endif // CARBIO_PROTOCOL_HANDLER_H
