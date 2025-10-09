#include "carbio/packet.h"
#include "carbio/endian.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>

namespace carbio
{
result<std::size_t>
packet::encode(std::span<std::uint8_t> buffer) noexcept
{
  const std::size_t packet_size = max_header_size + length + 2;
  if (buffer.size() < packet_size) return make_error(status_code::bad_packet);

  auto* ptr = buffer.data();
  const std::uint16_t length_with_checksum = length + 2;

  // Write header
  std::ranges::copy(to_bytes_be(tag), ptr); ptr += 2;
  std::ranges::copy(to_bytes_be(address), ptr); ptr += 4;
  *ptr++ = type;
  std::ranges::copy(to_bytes_be(length_with_checksum), ptr); ptr += 2;

  // Write data
  if (length > 0) {
    std::ranges::copy_n(data.begin(), length, ptr);
    ptr += length;
  }

  // Calculate and write checksum in one pass
  std::uint16_t checksum = type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF);
  checksum += std::accumulate(data.begin(), data.begin() + length, 0u);
  std::ranges::copy(to_bytes_be(checksum), ptr);
  
  return make_success(packet_size);
}

result<std::size_t>
packet::decode(std::span<const std::uint8_t> buffer, std::uint32_t expected_address) noexcept
{
  if (buffer.size() < max_header_size + 2) return make_error(status_code::frame_error);
  auto* ptr = buffer.data();

  // Read and validate header
  tag = read_be<std::uint16_t>(std::span(ptr, 2)); ptr += 2;
  if (tag != builtin_tag) return make_error(status_code::bad_packet);
  address = read_be<std::uint32_t>(std::span(ptr, 4)); ptr += 4;
  if (address != expected_address) return make_error(status_code::bad_packet);
  type = *ptr++;
  const std::uint16_t length_with_checksum = read_be<std::uint16_t>(std::span(ptr, 2)); ptr += 2;
  if (length_with_checksum < 2 || (length = length_with_checksum - 2) > max_data_size || buffer.size() < max_header_size + length_with_checksum) return make_error(status_code::bad_packet);

  // Read data and checksum
  if (length > 0) {
    std::ranges::copy_n(ptr, length, data.begin());
    ptr += length;
  }
  const std::uint16_t received_checksum = read_be<std::uint16_t>(std::span(ptr, 2));
  
  // Verify checksum
  const std::uint16_t calculated_checksum = type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF) + std::accumulate(data.begin(), data.begin() + length, 0u);
  if (received_checksum != calculated_checksum) return make_error(status_code::bad_packet);
  return make_success(max_header_size + length_with_checksum);
}
} // namespace carbio
