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

#ifndef CARBIO_FINGERPRINT_PROTOCOL_HANDLER_H
#define CARBIO_FINGERPRINT_PROTOCOL_HANDLER_H

#include "carbio/internal/io/task.h"
#include "carbio/internal/fingerprint/operation_code.h"
#include "carbio/internal/fingerprint/packet.h"
#include "carbio/internal/fingerprint/packet_deserializer.h"
#include "carbio/internal/fingerprint/packet_serializer.h"
#include "carbio/internal/fingerprint/packet_type.h"
#include "carbio/internal/fingerprint/result.h"
#include "carbio/internal/fingerprint/status_code.h"
#include "carbio/internal/io/serial_port.h"

#include <chrono>
#include <span>
#include <vector>

namespace carbio::internal
{

/*!
 * @brief Handles fingerprint sensor protocol communication
 *
 * Responsible for:
 * - Packet serialization/deserialization
 * - Send/receive operations over UART
 * - Packet validation
 */
class protocol_handler
{
public:
  static constexpr std::uint32_t DEFAULT_ADDRESS = 0xFFFFFFFF;
  static constexpr std::int32_t DEFAULT_TIMEOUT_MS = 1000;

  explicit protocol_handler(serial_port& serial, std::uint32_t device_address = DEFAULT_ADDRESS);

  /*!
   * @brief Send command packet and receive acknowledgment
   * @tparam N Packet size
   * @param opCode Operation code
   * @param data Command data payload
   * @param timeout Timeout in milliseconds
   * @returns Command result with status and response data
   */
  [[nodiscard]] task<result<std::vector<std::uint8_t>>>
  send_command(operation_code op_code, std::span<const std::uint8_t> data = {},
               std::chrono::milliseconds timeout = std::chrono::milliseconds{DEFAULT_TIMEOUT_MS});

  /*!
   * @brief Send packet to device
   * @tparam N Packet payload size
   * @param packet Packet to send
   * @param payloadSize Actual payload size
   * @returns Success or error
   */
  template<std::size_t N>
  [[nodiscard]] task<void_result> send_packet(basic_packet<N> const& packet, std::size_t payload_size);

  /*!
   * @brief Receive packet from device
   * @tparam N Expected packet payload size
   * @param timeout Timeout in milliseconds
   * @returns Received packet or error
   */
  template<std::size_t N>
  [[nodiscard]] task<result<basic_packet<N>>>
  receive_packet(std::chrono::milliseconds timeout = std::chrono::milliseconds{DEFAULT_TIMEOUT_MS});

  /*!
   * @brief Send data in multiple packets
   * @param data Data to send
   * @param packetSize Size of each packet
   * @returns Success or error
   */
  [[nodiscard]] task<void_result> send_data_packets(std::span<const std::uint8_t> data, std::size_t packet_size);

  /*!
   * @brief Receive data from multiple packets
   * @param timeout Timeout for each packet
   * @returns Received data or error
   */
  [[nodiscard]] task<result<std::vector<std::uint8_t>>>
  receive_data_packets(std::chrono::milliseconds timeout = std::chrono::milliseconds{DEFAULT_TIMEOUT_MS});

  void set_device_address(std::uint32_t address) { m_device_address = address; }
  [[nodiscard]] std::uint32_t device_address() const { return m_device_address; }

private:
  serial_port& m_serial;
  std::uint32_t m_device_address;
};

} // namespace carbio::internal

#endif // CARBIO_FINGERPRINT_PROTOCOL_HANDLER_H
