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

#ifndef CARBIO_PACKET_H
#define CARBIO_PACKET_H

#include "carbio/result.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace carbio
{
/*!
 * @brief The packet structure.
 */
struct packet
{
  inline static constexpr std::uint16_t   builtin_tag     = 0xEF01;
  inline static constexpr std::size_t     max_header_size = 9;
  inline static constexpr std::size_t     max_data_size   = 256;
  inline static constexpr std::size_t     max_packet_size = max_header_size + max_data_size + 2;
  std::uint16_t                           tag{builtin_tag};
  std::uint32_t                           address{0xFFFFFFFF};
  std::uint8_t                            type{0x01};
  std::uint16_t                           length{0};
  std::array<std::uint8_t, max_data_size> data{};
  result<std::size_t> decode(std::span<const std::uint8_t> buffer, std::uint32_t expected_address) noexcept;
  result<std::size_t> encode(std::span<std::uint8_t> buffer) noexcept;
};
} // namespace carbio

#endif // CARBIO_PACKET_H
