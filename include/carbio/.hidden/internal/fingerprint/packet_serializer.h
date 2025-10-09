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

#ifndef CARBIO_FINGERPRINT_PACKET_SERIALIZER_H
#define CARBIO_FINGERPRINT_PACKET_SERIALIZER_H

#include "carbio/internal/fingerprint/endian.h"
#include "carbio/internal/fingerprint/packet.h"

#include <span>
#include <vector>

namespace carbio::internal
{

/*!
 * @brief Serialize packet to byte buffer
 * @tparam N Packet payload capacity
 * @param pkt Packet to serialize
 * @param actualPayloadSize Actual size of payload data (may be < N)
 * @returns Serialized bytes (header + payload + checksum)
 */
template<std::size_t N>
[[nodiscard]] std::vector<std::uint8_t> serialize(basic_packet<N> const& pkt, std::size_t actualPayloadSize)
{
  constexpr std::size_t HEADER_SIZE = 9;
  std::vector<std::uint8_t> buffer;
  buffer.reserve(HEADER_SIZE + actualPayloadSize + 2);

  // Start code (2 bytes, big-endian)
  buffer.push_back(static_cast<std::uint8_t>(pkt.header.code >> 8));
  buffer.push_back(static_cast<std::uint8_t>(pkt.header.code & 0xFF));

  // Address (4 bytes, big-endian)
  buffer.push_back(static_cast<std::uint8_t>((pkt.header.address >> 24) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>((pkt.header.address >> 16) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>((pkt.header.address >> 8) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>(pkt.header.address & 0xFF));

  // Packet type (1 byte)
  buffer.push_back(pkt.header.type);

  // Length = payload size + checksum (2 bytes, big-endian)
  std::uint16_t const totalLength = static_cast<std::uint16_t>(actualPayloadSize + 2);
  buffer.push_back(static_cast<std::uint8_t>(totalLength >> 8));
  buffer.push_back(static_cast<std::uint8_t>(totalLength & 0xFF));

  // Payload data
  for (std::size_t i = 0; i < actualPayloadSize; ++i)
  {
    buffer.push_back(pkt.payload.data[i]);
  }

  // Calculate checksum: type + length_hi + length_lo + all payload bytes
  std::uint16_t checksum = 0;
  checksum += pkt.header.type;
  checksum += static_cast<std::uint8_t>(totalLength >> 8);
  checksum += static_cast<std::uint8_t>(totalLength & 0xFF);
  for (std::size_t i = 0; i < actualPayloadSize; ++i)
  {
    checksum += pkt.payload.data[i];
  }

  // Checksum (2 bytes, big-endian)
  buffer.push_back(static_cast<std::uint8_t>(checksum >> 8));
  buffer.push_back(static_cast<std::uint8_t>(checksum & 0xFF));

  return buffer;
}

} // namespace carbio::internal

#endif // CARBIO_FINGERPRINT_PACKET_SERIALIZER_H
