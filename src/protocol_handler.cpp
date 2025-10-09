#include "carbio/protocol_handler.h"
#include "carbio/packet.h"

#include <algorithm>
#include <ranges>

namespace carbio
{

/*!
   * @brief Construct a command packet.
   * @param code Command code.
   * @param data Command parameters.
   * @returns Response from device.
   */
result<std::vector<std::uint8_t>>
protocol_handler::construct_command_packet(command_code code, std::span<const std::uint8_t> data) const noexcept
{
  packet p{ .address = address_, .type = static_cast<std::uint8_t>(packet_id::command) };
  p.data[0] = static_cast<std::uint8_t>(code); // set command code as the first byte in payload
  p.length = 1;
  if (!data.empty()) // add command params
  {
    std::size_t n = std::ranges::min(data.size(), packet::max_data_size - 1);
    std::ranges::copy_n(data.begin(), n, p.data.begin() + 1);
    p.length += static_cast<std::uint16_t>(n);
  }
  std::vector<std::uint8_t> buffer(packet::max_header_size + p.length + 2);
  auto result = p.encode(buffer); // encode packet
  if (!result) return make_error(status_code::no_frame);
  buffer.resize(*result); // trim to actual size
  return buffer;
}

/*!
   * @brief Parse acknowledge packet.
   * @param data 
   * @returns Response from device.
   */
result<std::vector<std::uint8_t>>
protocol_handler::parse_acknowledge_packet(std::span<const std::uint8_t> data) const noexcept
{
  packet p{};
  auto result = p.decode(data, address_);
  if (!result) return make_error(status_code::no_frame);
  if (static_cast<std::uint8_t>(packet_id::acknowledge) != p.type) return make_error(status_code::no_frame);
  if (0 == p.length) return make_error(status_code::no_frame);
  auto status = static_cast<status_code>(p.data[0]);
  if (is_error(status)) return make_error(status);
  std::vector<std::uint8_t> response;
  if (1<p.length) response.assign(p.data.begin()+1, p.data.begin()+p.length);
  return make_success(response);
}

/*!
   * @brief Construct a data packet.
   * @param data Byte stream. 
   * @returns Response from device.
   */
result<std::vector<std::vector<std::uint8_t>>>
protocol_handler::construct_data_packet(std::span<const std::uint8_t> data) const noexcept
{
  std::vector<std::vector<std::uint8_t>> ret;
  std::size_t offset = 0;
  while (offset < data.size())
  {
    std::array<std::size_t, 3> sizes{length_,data.size() - offset,static_cast<std::size_t>(packet::max_data_size)};
    std::size_t chunk_size = std::ranges::min(sizes);
    auto is_last = (offset + chunk_size >= data.size());
    packet p;
    p.address = address_;
    p.type = static_cast<std::uint8_t>(is_last ? packet_id::end_data : packet_id::data);
    p.length = static_cast<std::uint16_t>(chunk_size);
    std::ranges::copy_n(data.begin() + offset, chunk_size, p.data.begin());
    std::vector<std::uint8_t> buffer(packet::max_header_size + p.length + 2);
    auto result = p.encode(buffer);
    if (result)
    {
      buffer.resize(*result); // trim to actual size
      ret.push_back(std::move(buffer));
    }
    offset += chunk_size;
  }
  return ret;
}

/*!
   * @brief Parse a data packet.
   * @param data Byte stream.
   * @returns Response from device.
   */
result<std::vector<std::uint8_t>>
protocol_handler::parse_data_packet(std::span<const std::span<std::uint8_t>> data) const noexcept
{
  std::vector<std::uint8_t> ret;
  for (auto const& e : data)
  {
    packet p;
    auto result = p.decode(e, address_);
    if (!result) return make_error(status_code::no_frame);
    if (p.type != static_cast<std::uint8_t>(packet_id::data) && p.type != static_cast<std::uint8_t>(packet_id::end_data)) return make_error(status_code::no_frame);
    ret.insert(ret.end(), p.data.begin(), p.data.begin() + p.length);
    if (p.type == static_cast<std::uint8_t>(packet_id::end_data)) break;
  }
  return ret;
}

} // namespace carbio
