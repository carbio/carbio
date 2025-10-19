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
#include "carbio/fingerprint/protocol_handler.h"

#include <algorithm>
#include <ranges>

namespace carbio::fingerprint {

/*!
 * @brief Construct a command packet (using secure buffer).
 * @param code Command code.
 * @param data Command parameters.
 * @returns Secure command packet or error.
 */
result<locked_buffer<std::uint8_t>> protocol_handler::construct_command_packet(
    command_code code, std::span<const std::uint8_t> data) const noexcept {
  packet p;
  p.address = address_;
  p.type = static_cast<std::uint8_t>(packet_id::command);
  p.data[0] = static_cast<std::uint8_t>(
      code); // set command code as the first byte in payload
  p.length = 1;
  if (!data.empty()) // add command params
  {
    std::size_t n = std::ranges::min(data.size(), packet::max_data_size - 1);
    std::ranges::copy_n(data.begin(), n, p.data.begin() + 1);
    p.length += static_cast<std::uint16_t>(n);
  }
  locked_buffer<std::uint8_t> buffer(packet::max_header_size + p.length + 2);
  auto result = p.encode(buffer.as_span()); // encode packet
  if (!result)
    return make_error(status_code::no_frame);
  // Resize to actual size (create new buffer with correct size)
  locked_buffer<std::uint8_t> trimmed(*result);
  trimmed.copy_from(buffer.data(), *result);
  return make_success(std::move(trimmed));
}

/*!
 * @brief Parse acknowledge packet (using secure buffer).
 * @param data Input packet data
 * @returns Secure response data or error.
 */
result<locked_buffer<std::uint8_t>> protocol_handler::parse_acknowledge_packet(
    std::span<const std::uint8_t> data) const noexcept {
  packet p{};
  auto result = p.decode(data, address_);
  if (!result)
    return make_error(status_code::no_frame);
  if (static_cast<std::uint8_t>(packet_id::acknowledge) != p.type)
    return make_error(status_code::no_frame);
  if (0 == p.length)
    return make_error(status_code::no_frame);
  auto status = static_cast<status_code>(p.data[0]);
  if (is_error(status))
    return make_error(status);
  locked_buffer<std::uint8_t> response;
  if (1 < p.length) {
    if (!response.resize(p.length - 1))
      return make_error(status_code::no_frame);
    response.copy_from(p.data.data() + 1, p.length - 1);
  }
  return make_success(std::move(response));
}

/*!
 * @brief Construct data packets (using secure buffers).
 * @param data Byte stream to packetize.
 * @returns Vector of secure data packets or error.
 */
result<std::vector<locked_buffer<std::uint8_t>>>
protocol_handler::construct_data_packet(
    std::span<const std::uint8_t> data) const noexcept {
  std::vector<locked_buffer<std::uint8_t>> ret;
  std::size_t offset = 0;
  while (offset < data.size()) {
    std::array<std::size_t, 3> sizes{
        length_, data.size() - offset,
        static_cast<std::size_t>(packet::max_data_size)};
    std::size_t chunk_size = std::ranges::min(sizes);
    auto is_last = (offset + chunk_size >= data.size());
    packet p;
    p.address = address_;
    p.type = static_cast<std::uint8_t>(is_last ? packet_id::end_data
                                               : packet_id::data);
    p.length = static_cast<std::uint16_t>(chunk_size);
    std::ranges::copy_n(data.begin() + offset, chunk_size, p.data.begin());
    locked_buffer<std::uint8_t> buffer(packet::max_header_size + p.length + 2);
    auto result = p.encode(buffer.as_span());
    if (result) {
      // Resize to actual size
      locked_buffer<std::uint8_t> trimmed(*result);
      trimmed.copy_from(buffer.data(), *result);
      ret.push_back(std::move(trimmed));
    }
    offset += chunk_size;
  }
  return make_success(std::move(ret));
}

/*!
 * @brief Parse data packets (using secure buffer).
 * @param data Byte stream.
 * @returns Secure accumulated data or error.
 */
result<locked_buffer<std::uint8_t>> protocol_handler::parse_data_packet(
    std::span<const std::span<std::uint8_t>> data) const noexcept {
  locked_buffer<std::uint8_t> ret;
  std::size_t total_size = 0;

  for (auto const &e : data) {
    packet p;
    auto result = p.decode(e, address_);
    if (!result)
      return make_error(status_code::no_frame);
    if (p.type != static_cast<std::uint8_t>(packet_id::data) &&
        p.type != static_cast<std::uint8_t>(packet_id::end_data))
      return make_error(status_code::no_frame);

    // Securely accumulate data
    std::size_t new_size = total_size + p.length;
    locked_buffer<std::uint8_t> new_buffer(new_size);
    if (total_size > 0) {
      new_buffer.copy_from(ret.data(), total_size);
    }
    std::memcpy(new_buffer.data() + total_size, p.data.data(), p.length);
    ret = std::move(new_buffer);
    total_size = new_size;

    if (p.type == static_cast<std::uint8_t>(packet_id::end_data))
      break;
  }
  return make_success(std::move(ret));
}

} // namespace carbio::fingerprint
