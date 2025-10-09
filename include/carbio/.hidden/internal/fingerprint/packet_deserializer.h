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

#ifndef CARBIO_FINGERPRINT_PACKET_DESERIALIZER_H
#define CARBIO_FINGERPRINT_PACKET_DESERIALIZER_H

#include "carbio/internal/fingerprint/endian.h"
#include "carbio/internal/fingerprint/packet.h"
#include "carbio/internal/fingerprint/result.h"
#include "carbio/internal/fingerprint/status_code.h"

#include <optional>
#include <span>

namespace carbio::internal
{

constexpr std::size_t MIN_PACKET_SIZE = 12; // Header (9) + min payload (1) + checksum (2)
constexpr std::uint16_t PACKET_START_CODE = 0xEF01;

/*!
 * @brief Deserialize packet from byte buffer
 * @tparam N Expected packet payload capacity
 * @param buffer Input byte buffer
 * @param expectedAddress Expected device address (for validation)
 * @returns Deserialized packet or error
 */
template<std::size_t N>
[[nodiscard]] result<basic_packet<N>> deserialize(std::span<const std::uint8_t> buffer, std::uint32_t expected_address)
{
  if (buffer.size() < MIN_PACKET_SIZE)
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  // Parse header
  std::uint16_t start_code = read_be<std::uint16_t>(buffer.subspan(0, 2));
  if (start_code != PACKET_START_CODE)
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  std::uint32_t address = read_be<std::uint32_t>(buffer.subspan(2, 4));
  if (address != expected_address)
  {
    return make_error<basic_packet<N>>(status_code::illegal_device_address);
  }

  std::uint8_t packet_type = buffer[6];
  std::uint16_t length = read_be<std::uint16_t>(buffer.subspan(7, 2));

  if (length < 2) // Must include at least checksum
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  constexpr std::size_t HEADER_SIZE = 9;
  std::size_t total_size = HEADER_SIZE + length;
  if (buffer.size() < total_size)
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  // Extract payload
  std::size_t data_len = length - 2; // Subtract checksum
  if (data_len > N)
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  basic_packet<N> pkt;
  pkt.header.code = start_code;
  pkt.header.address = address;
  pkt.header.type = packet_type;
  pkt.header.length = length;

  // Copy payload data
  for (std::size_t i = 0; i < data_len; ++i)
  {
    pkt.payload.data[i] = buffer[HEADER_SIZE + i];
  }

  // Extract and verify checksum
  std::uint16_t received_checksum = read_be<std::uint16_t>(buffer.subspan(HEADER_SIZE + data_len, 2));

  std::uint16_t calculated_checksum = 0;
  calculated_checksum += packet_type;
  calculated_checksum += static_cast<std::uint8_t>(length >> 8);
  calculated_checksum += static_cast<std::uint8_t>(length & 0xFF);
  for (std::size_t i = 0; i < data_len; ++i)
  {
    calculated_checksum += pkt.payload.data[i];
  }

  if (received_checksum != calculated_checksum)
  {
    return make_error<basic_packet<N>>(status_code::bad_packet);
  }

  pkt.payload.checksum = received_checksum;
  return make_success(std::move(pkt));
}

} // namespace carbio::internal

#endif // CARBIO_FINGERPRINT_PACKET_DESERIALIZER_H
