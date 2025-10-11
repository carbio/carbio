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

#ifndef CARBIO_COMMAND_EXECUTOR_H
#define CARBIO_COMMAND_EXECUTOR_H

#include "carbio/command_code.h"
#include "carbio/command_traits.h"
#include "carbio/packet.h"
#include "carbio/protocol_handler.h"
#include "carbio/result.h"
#include "carbio/serial_port.h"
#include "carbio/command_serializer.h"

#include <chrono>
#include <optional>
#include <span>
#include <vector>

namespace carbio
{
/*!
 * @brief Executes commands using type-safe command_traits.
 * Works with the carbio::serial_port class and uses fixed-size buffers.
 */
class command_executor
{
  serial_port      &serial_;
  protocol_handler &protocol_;

public:
  explicit command_executor(serial_port &serial, protocol_handler &protocol) noexcept
      : serial_(serial), protocol_(protocol)
  {
  }

  /*!
   * @brief Execute a command with type-safe request/response.
   * @tparam code Command code
   * @param request Command request data
   * @return Command response wrapped in result<T>
   */
  template<command_code code>
  [[nodiscard]] typename command_traits<code>::response
  execute(typename command_traits<code>::request const& request) noexcept
  {
    // Flush any stale data
    serial_.flush();

    // Serialize request
    auto request_data = serialize_request<code>(request);

    // Build command packet
    auto cmd_packet_result = protocol_.construct_command_packet(code, request_data);
    if (!cmd_packet_result)
    {
      return make_error(cmd_packet_result.error());
    }

    // Send packet
    auto const& cmd_packet = *cmd_packet_result;
    std::size_t written = serial_.write_exact(cmd_packet);
    if (written != cmd_packet.size())
    {
      return make_error(status_code::timeout);
    }

    // Wait for transmission to complete
    serial_.drain();

    // Receive response - read header first
    std::vector<std::uint8_t> header_buffer(packet::max_header_size);
    std::size_t header_read = serial_.read_exact(header_buffer);
    if (header_read < packet::max_header_size) return make_error(status_code::frame_error);

    // Parse length from header (bytes 7-8)
    std::uint16_t length_with_checksum = (static_cast<std::uint16_t>(header_buffer[7]) << 8) |
                                         static_cast<std::uint16_t>(header_buffer[8]);
    if (length_with_checksum < 2) return make_error(status_code::bad_packet);

    // Read remaining data + checksum
    std::vector<std::uint8_t> full_packet(packet::max_header_size + length_with_checksum);
    std::copy(header_buffer.begin(), header_buffer.end(), full_packet.begin());
    std::size_t body_read = serial_.read_exact(std::span{full_packet.data() + packet::max_header_size,
                                                length_with_checksum});
    if (body_read < length_with_checksum) return make_error(status_code::frame_error);

    // Parse acknowledgment
    auto ack_result = protocol_.parse_acknowledge_packet(full_packet);
    if (!ack_result)
    {
      return make_error(ack_result.error());
    }

    // Deserialize response
    return deserialize_response<code>(ack_result->empty() ? std::span<const std::uint8_t>{} : std::span{*ack_result});
  }

  /*!
   * @brief Send data packets for upload operations.
   * @param data Data to upload
   * @return void_result indicating success or error
   */
  [[nodiscard]] void_result send_data_packets(std::span<const std::uint8_t> data) noexcept
  {
    auto packets_result = protocol_.construct_data_packet(data);
    if (!packets_result)
    {
      return make_error(packets_result.error());
    }

    for (auto const& packet : *packets_result)
    {
      std::size_t written = serial_.write_exact(packet);
      if (written != packet.size()) return make_error(status_code::timeout);
    }

    serial_.drain();
    return make_success();
  }

  /*!
   * @brief Receive data packets for download operations.
   * @return Downloaded data or error
   */
  [[nodiscard]] result<std::vector<std::uint8_t>> receive_data_packets() noexcept
  {
    std::vector<std::uint8_t> ret;
    while (true)
    {
      // Read header first
      std::vector<std::uint8_t> header(packet::max_header_size);
      std::size_t header_read = serial_.read_exact(header);
      if (header_read < packet::max_header_size) return make_error(status_code::frame_error);

      // Parse length from header (bytes 7-8)
      std::uint16_t length_with_checksum = (static_cast<std::uint16_t>(header[7]) << 8) |
                                           static_cast<std::uint16_t>(header[8]);
      if (length_with_checksum < 2) return make_error(status_code::bad_packet);

      // Read remaining data + checksum
      std::vector<std::uint8_t> full_packet(packet::max_header_size + length_with_checksum);
      std::copy(header.begin(), header.end(), full_packet.begin());
      std::size_t body_read = serial_.read_exact(std::span{full_packet.data() + packet::max_header_size,
                                                  length_with_checksum});
      if (body_read < length_with_checksum) return make_error(status_code::frame_error);

      // Parse packet
      packet p;
      auto parsed = p.decode(full_packet, protocol_.get_address());
      if (!parsed) return make_error(parsed.error());

      bool is_last = (p.type == static_cast<std::uint8_t>(packet_id::end_data));
      ret.insert(ret.end(), p.data.begin(), p.data.begin() + p.length);

      if (is_last) break;
    }
    return make_success(std::move(ret));
  }

  command_executor(const command_executor &)            = delete;
  command_executor &operator=(const command_executor &) = delete;
  command_executor(command_executor &&)                 = default;
  command_executor &operator=(command_executor &&)      = delete; // Contains references
};
} /* namespace carbio */

#endif // CARBIO_COMMAND_EXECUTOR_H