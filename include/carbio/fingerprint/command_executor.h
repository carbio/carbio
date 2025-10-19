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

#include "carbio/fingerprint/command_code.h"
#include "carbio/fingerprint/command_serializer.h"
#include "carbio/fingerprint/command_traits.h"
#include "carbio/fingerprint/packet.h"
#include "carbio/fingerprint/protocol_handler.h"
#include "carbio/fingerprint/result.h"
#include "carbio/io/serial_port.h"
#include "carbio/utility/locked_buffer.h"

#include <chrono>
#include <optional>
#include <span>
#include <vector>

namespace carbio::fingerprint {
/*!
 * @brief Executes commands using type-safe command_traits.
 * Works with the carbio::io::serial_port class and uses fixed-size buffers.
 */
class command_executor {
  io::serial_port &serial_;
  protocol_handler &protocol_;

public:
  explicit command_executor(io::serial_port &serial,
                            protocol_handler &protocol) noexcept
      : serial_(serial), protocol_(protocol) {}

  /*!
   * @brief Execute a command with type-safe request/response.
   * @tparam code Command code
   * @param request Command request data
   * @return Command response wrapped in result<T>
   */
  template <command_code code>
  [[nodiscard]] typename command_traits<code>::response
  execute(typename command_traits<code>::request const &request) noexcept {
    // Flush any stale data
    serial_.flush();

    // Serialize request
    auto request_data = serialize_request<code>(request);

    // Build command packet
    auto cmd_packet_result =
        protocol_.construct_command_packet(code, request_data);
    if (!cmd_packet_result) {
      return make_error(cmd_packet_result.error());
    }

    // Send packet
    auto const &cmd_packet = *cmd_packet_result;
    std::size_t written = serial_.write_exact(cmd_packet);
    if (written != cmd_packet.size()) {
      return make_error(status_code::timeout);
    }

    // Wait for transmission to complete
    serial_.drain();

    // Receive response - read header first (using secure buffer)
    locked_buffer<std::uint8_t> header_buffer(packet::max_header_size);
    std::size_t header_read = serial_.read_exact(header_buffer.as_span());
    if (header_read < packet::max_header_size)
      return make_error(status_code::frame_error);

    // Parse length from header (bytes 7-8)
    std::uint16_t length_with_checksum =
        (static_cast<std::uint16_t>(header_buffer[7]) << 8) |
        static_cast<std::uint16_t>(header_buffer[8]);
    if (length_with_checksum < 2)
      return make_error(status_code::bad_packet);

    // Read remaining data + checksum (using secure buffer)
    locked_buffer<std::uint8_t> full_packet(packet::max_header_size +
                                             length_with_checksum);
    std::copy(header_buffer.begin(), header_buffer.end(), full_packet.begin());
    std::size_t body_read = serial_.read_exact(std::span{
        full_packet.data() + packet::max_header_size, length_with_checksum});
    if (body_read < length_with_checksum)
      return make_error(status_code::frame_error);

    // Parse acknowledgment
    auto ack_result = protocol_.parse_acknowledge_packet(full_packet.as_span());
    if (!ack_result) {
      return make_error(ack_result.error());
    }

    // Deserialize response
    return deserialize_response<code>(ack_result->empty()
                                          ? std::span<const std::uint8_t>{}
                                          : std::span{*ack_result});
  }

  /*!
   * @brief Send data packets for upload operations.
   * @param data Data to upload
   * @return void_result indicating success or error
   */
  [[nodiscard]] void_result
  send_data_packets(std::span<const std::uint8_t> data) noexcept {
    auto packets_result = protocol_.construct_data_packet(data);
    if (!packets_result) {
      return make_error(packets_result.error());
    }

    for (auto const &packet : *packets_result) {
      std::size_t written = serial_.write_exact(packet);
      if (written != packet.size())
        return make_error(status_code::timeout);
    }

    serial_.drain();
    return make_success();
  }

  /*!
   * @brief Receive data packets for download operations (using secure buffer).
   * @return Downloaded data in locked_buffer or error
   */
  [[nodiscard]] result<locked_buffer<std::uint8_t>>
  receive_data_packets() noexcept {
    locked_buffer<std::uint8_t> ret;
    std::size_t total_size = 0;

    while (true) {
      // Read header first (using secure buffer)
      locked_buffer<std::uint8_t> header(packet::max_header_size);
      std::size_t header_read = serial_.read_exact(header.as_span());
      if (header_read < packet::max_header_size)
        return make_error(status_code::frame_error);

      // Parse length from header (bytes 7-8)
      std::uint16_t length_with_checksum =
          (static_cast<std::uint16_t>(header[7]) << 8) |
          static_cast<std::uint16_t>(header[8]);
      if (length_with_checksum < 2)
        return make_error(status_code::bad_packet);

      // Read remaining data + checksum (using secure buffer)
      locked_buffer<std::uint8_t> full_packet(packet::max_header_size +
                                               length_with_checksum);
      std::copy(header.begin(), header.end(), full_packet.begin());
      std::size_t body_read = serial_.read_exact(std::span{
          full_packet.data() + packet::max_header_size, length_with_checksum});
      if (body_read < length_with_checksum)
        return make_error(status_code::frame_error);

      // Parse packet
      packet p;
      auto parsed = p.decode(full_packet.as_span(), protocol_.get_address());
      if (!parsed)
        return make_error(parsed.error());

      bool is_last = (p.type == static_cast<std::uint8_t>(packet_id::end_data));

      // Securely accumulate data by resizing and copying
      std::size_t new_size = total_size + p.length;
      locked_buffer<std::uint8_t> new_buffer(new_size);
      if (total_size > 0) {
        new_buffer.copy_from(ret.data(), total_size);
      }
      std::memcpy(new_buffer.data() + total_size, p.data.data(), p.length);
      ret = std::move(new_buffer);
      total_size = new_size;

      if (is_last)
        break;
    }
    return make_success(std::move(ret));
  }

  command_executor(const command_executor &) = delete;
  command_executor &operator=(const command_executor &) = delete;
  command_executor(command_executor &&) = default;
  command_executor &
  operator=(command_executor &&) = delete; // Contains references
};
} // namespace carbio::fingerprint
