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

#include "carbio/fingerprint/packet.h"
#include "carbio/utility/endian.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>

namespace carbio::fingerprint {
result<std::size_t> packet::encode(std::span<std::uint8_t> buffer) noexcept {
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  const std::size_t packet_size = max_header_size + length + 2;
  if (buffer.size() < packet_size)
    return make_error(status_code::bad_packet);

  auto *ptr = buffer.data();
  const std::uint16_t length_with_checksum = length + 2;

  // Write header
  std::ranges::copy(carbio::utility::to_bytes_be(tag), ptr);
  ptr += 2;
  std::ranges::copy(carbio::utility::to_bytes_be(address), ptr);
  ptr += 4;
  *ptr++ = type;
  std::ranges::copy(carbio::utility::to_bytes_be(length_with_checksum), ptr);
  ptr += 2;

  // Write data
  if (length > 0) {
    std::ranges::copy_n(data.begin(), length, ptr);
    ptr += length;
  }

  // Calculate and write checksum in one pass
  std::uint16_t checksum = static_cast<std::uint16_t>(
      type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF));
  checksum = static_cast<std::uint16_t>(
      checksum + std::accumulate(data.begin(), data.begin() + length, 0u));
  std::ranges::copy(carbio::utility::to_bytes_be(checksum), ptr);

  return make_success(packet_size);
}

result<std::size_t> packet::decode(std::span<const std::uint8_t> buffer,
                                   std::uint32_t expected_address) noexcept {
  if (buffer.size() < max_header_size + 2)
    return make_error(status_code::frame_error);
  auto *ptr = buffer.data();

  // Read and validate header
  tag = carbio::utility::read_be<std::uint16_t>(std::span(ptr, 2));
  ptr += 2;
  if (tag != builtin_tag)
    return make_error(status_code::bad_packet);
  address = carbio::utility::read_be<std::uint32_t>(std::span(ptr, 4));
  ptr += 4;
  if (address != expected_address)
    return make_error(status_code::bad_packet);
  type = *ptr++;
  const std::uint16_t length_with_checksum =
      carbio::utility::read_be<std::uint16_t>(std::span(ptr, 2));
  ptr += 2;
  if (length_with_checksum < 2 ||
      (length = length_with_checksum - 2) > max_data_size ||
      buffer.size() < max_header_size + length_with_checksum)
    return make_error(status_code::bad_packet);

  // Read data and checksum
  if (length > 0) {
    std::ranges::copy_n(ptr, length, data.begin());
    ptr += length;
  }
  const std::uint16_t received_checksum =
      carbio::utility::read_be<std::uint16_t>(std::span(ptr, 2));

  // Verify checksum
  const std::uint16_t calculated_checksum = static_cast<std::uint16_t>(
      type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF) +
      std::accumulate(data.begin(), data.begin() + length, 0u));
  if (received_checksum != calculated_checksum)
    return make_error(status_code::bad_packet);
  return make_success(max_header_size + length_with_checksum);
}
} // namespace carbio::fingerprint
