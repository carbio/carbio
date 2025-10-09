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

#ifndef CARBIO_FINGERPRINT_PACKET_H
#define CARBIO_FINGERPRINT_PACKET_H

#include <array>
#include <cstdint>
#include <variant>

namespace carbio::internal
{
/*!
 * @brief Packet header structure (9 bytes)
 */
struct packet_header
{
  std::uint16_t code{0xEF01};       /*!< Packet start code (0xEF01) */
  std::uint32_t address{0xFFFFFFFF}; /*!< Device address */
  std::uint8_t  type{0x01};          /*!< Packet type (see packet_type enum) */
  std::uint16_t length{0};           /*!< Payload length + checksum (2 bytes) */
};

/*!
 * @brief Packet payload with fixed capacity
 */
template<std::size_t N>
struct packet_payload
{
  inline static constexpr std::size_t capacity = N;
  std::array<std::uint8_t, N> data{}; /*!< Packet data */
  std::uint16_t checksum{0};          /*!< 16-bit checksum */
};

/*!
 * @brief Complete packet structure (header + payload)
 */
template<std::size_t N>
struct basic_packet
{
  packet_header     header;
  packet_payload<N> payload;
};

using packet = std::variant<basic_packet<32>, basic_packet<64>, basic_packet<128>, basic_packet<256>>;

// Type aliases for convenience
using packet32 = basic_packet<32>;
using packet64 = basic_packet<64>;
using packet128 = basic_packet<128>;
using packet256 = basic_packet<256>;

} // namespace carbio::internal

#endif // CARBIO_FINGERPRINT_PACKET_H
