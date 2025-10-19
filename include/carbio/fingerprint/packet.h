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

#include "carbio/fingerprint/result.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace carbio::fingerprint {
/*!
 * @brief The packet structure with secure clearing.
 *
 * This packet structure automatically secures clears sensitive data
 * (fingerprint templates, images, match scores) on destruction.
 */
struct packet {
  inline static constexpr std::uint16_t builtin_tag = 0xEF01;
  inline static constexpr std::size_t max_header_size = 9;
  inline static constexpr std::size_t max_data_size = 256;
  inline static constexpr std::size_t max_packet_size =
      max_header_size + max_data_size + 2;
  std::uint16_t tag{builtin_tag};
  std::uint32_t address{0xFFFFFFFF};
  std::uint8_t type{0x01};
  std::uint16_t length{0};
  std::array<std::uint8_t, max_data_size> data{};

  /*!
   * @brief Default constructor.
   */
  packet() noexcept = default;

  /*!
   * @brief Destructor - securely clears data buffer.
   */
  ~packet() noexcept {
    secure_clear();
  }

  /*!
   * @brief Move constructor - clears source after move.
   */
  packet(packet &&other) noexcept
      : tag(other.tag), address(other.address), type(other.type),
        length(other.length), data(other.data) {
    other.secure_clear();
  }

  /*!
   * @brief Move assignment - clears both source and destination.
   */
  packet &operator=(packet &&other) noexcept {
    if (this != &other) {
      secure_clear();
      tag = other.tag;
      address = other.address;
      type = other.type;
      length = other.length;
      data = other.data;
      other.secure_clear();
    }
    return *this;
  }

  // Disable copy to prevent uncleared copies
  packet(const packet &) = delete;
  packet &operator=(const packet &) = delete;

  /*!
   * @brief Securely clear the data buffer.
   */
  void secure_clear() noexcept {
    volatile std::uint8_t *vptr = data.data();
    for (std::size_t i = 0; i < data.size(); ++i) {
      vptr[i] = 0;
    }
    length = 0;
  }

  result<std::size_t> decode(std::span<const std::uint8_t> buffer,
                             std::uint32_t expected_address) noexcept;
  result<std::size_t> encode(std::span<std::uint8_t> buffer) noexcept;
};
} // namespace carbio::fingerprint
