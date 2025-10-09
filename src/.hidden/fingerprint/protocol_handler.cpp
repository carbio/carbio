#include "carbio/internal/fingerprint/protocol_handler.h"

namespace carbio::internal
{

protocol_handler::protocol_handler(serial_port& serial, std::uint32_t device_address)
    : m_serial(serial), m_device_address(device_address)
{
}

task<result<std::vector<std::uint8_t>>> protocol_handler::send_command(operation_code op_code,
                                                                        std::span<const std::uint8_t> data,
                                                                        std::chrono::milliseconds timeout)
{
  // Build command packet
  packet128 cmd_packet;
  cmd_packet.header.code = 0xEF01;
  cmd_packet.header.address = m_device_address;
  cmd_packet.header.type = static_cast<std::uint8_t>(packet_type::command);

  // Pack operation code + data
  cmd_packet.payload.data[0] = static_cast<std::uint8_t>(op_code);
  std::size_t payload_size = 1;

  if (!data.empty())
  {
    if (data.size() > cmd_packet.payload.capacity - 1)
    {
      co_return make_error<std::vector<std::uint8_t>>(status_code::bad_packet);
    }
    std::copy(data.begin(), data.end(), cmd_packet.payload.data.begin() + 1);
    payload_size += data.size();
  }

  // Send packet
  auto send_result = co_await send_packet(cmd_packet, payload_size);
  if (!send_result)
  {
    co_return make_error<std::vector<std::uint8_t>>(send_result.error());
  }

  // Receive acknowledgment
  auto recv_result = co_await receive_packet<128>(timeout);
  if (!recv_result)
  {
    co_return make_error<std::vector<std::uint8_t>>(recv_result.error());
  }

  auto& ack_packet = *recv_result;

  // Validate acknowledgment
  if (ack_packet.header.type != static_cast<std::uint8_t>(packet_type::acknowledge))
  {
    co_return make_error<std::vector<std::uint8_t>>(status_code::bad_packet);
  }

  // Extract payload data
  std::size_t data_len = ack_packet.header.length - 2; // Subtract checksum
  std::vector<std::uint8_t> response_data(ack_packet.payload.data.begin(),
                                          ack_packet.payload.data.begin() + data_len);

  co_return make_success(std::move(response_data));
}

template<std::size_t N>
task<void_result> protocol_handler::send_packet(basic_packet<N> const& packet, std::size_t payload_size)
{
  if (!m_serial.is_open())
  {
    co_return make_error(status_code::communication_error);
  }

  auto buffer = serialize(packet, payload_size);
  auto write_result = m_serial.write(buffer);

  if (!write_result || *write_result < buffer.size())
  {
    co_return make_error(status_code::communication_error);
  }

  co_return make_success();
}

template<std::size_t N>
task<result<basic_packet<N>>> protocol_handler::receive_packet(std::chrono::milliseconds timeout)
{
  if (!m_serial.is_open())
  {
    co_return make_error<basic_packet<N>>(status_code::communication_error);
  }

  // Read minimum packet size first
  std::vector<std::uint8_t> buffer(MIN_PACKET_SIZE);
  auto read_result = m_serial.read_exact(buffer, timeout);

  if (!read_result || *read_result < MIN_PACKET_SIZE)
  {
    co_return make_error<basic_packet<N>>(status_code::timeout);
  }

  // Parse header to get total size
  std::uint16_t length = read_be<std::uint16_t>(std::span{buffer.data() + 7, 2});
  std::size_t total_size = 9 + length; // Header + length

  // Read remaining data if needed
  if (buffer.size() < total_size)
  {
    std::size_t remaining = total_size - buffer.size();
    buffer.resize(total_size);
    auto extra_result = m_serial.read_exact(std::span{buffer.data() + MIN_PACKET_SIZE, remaining}, timeout);

    if (!extra_result || *extra_result < remaining)
    {
      co_return make_error<basic_packet<N>>(status_code::timeout);
    }
  }

  // Deserialize packet
  auto deserialize_result = deserialize<N>(buffer, m_device_address);
  co_return deserialize_result;
}

task<void_result> protocol_handler::send_data_packets(std::span<const std::uint8_t> data, std::size_t packet_size)
{
  std::size_t offset = 0;

  while (offset < data.size())
  {
    std::size_t chunk_size = std::min(packet_size, data.size() - offset);
    bool is_last_packet = (offset + chunk_size >= data.size());

    packet256 data_packet;
    data_packet.header.code = 0xEF01;
    data_packet.header.address = m_device_address;
    data_packet.header.type = is_last_packet ? static_cast<std::uint8_t>(packet_type::end_data)
                                          : static_cast<std::uint8_t>(packet_type::data);

    std::copy(data.begin() + offset, data.begin() + offset + chunk_size, data_packet.payload.data.begin());

    auto send_result = co_await send_packet(data_packet, chunk_size);
    if (!send_result)
    {
      co_return send_result;
    }

    offset += chunk_size;
  }

  co_return make_success();
}

task<result<std::vector<std::uint8_t>>> protocol_handler::receive_data_packets(std::chrono::milliseconds timeout)
{
  std::vector<std::uint8_t> all_data;

  while (true)
  {
    auto packet_result = co_await receive_packet<256>(timeout);
    if (!packet_result)
    {
      co_return make_error<std::vector<std::uint8_t>>(packet_result.error());
    }

    auto& packet = *packet_result;

    if (packet.header.type != static_cast<std::uint8_t>(packet_type::data) &&
        packet.header.type != static_cast<std::uint8_t>(packet_type::end_data))
    {
      co_return make_error<std::vector<std::uint8_t>>(status_code::bad_packet);
    }

    std::size_t data_len = packet.header.length - 2;
    all_data.insert(all_data.end(), packet.payload.data.begin(), packet.payload.data.begin() + data_len);

    if (packet.header.type == static_cast<std::uint8_t>(packet_type::end_data))
    {
      break;
    }
  }

  co_return make_success(std::move(all_data));
}

// Explicit template instantiations
template task<void_result> protocol_handler::send_packet<32>(basic_packet<32> const&, std::size_t);
template task<void_result> protocol_handler::send_packet<64>(basic_packet<64> const&, std::size_t);
template task<void_result> protocol_handler::send_packet<128>(basic_packet<128> const&, std::size_t);
template task<void_result> protocol_handler::send_packet<256>(basic_packet<256> const&, std::size_t);

template task<result<basic_packet<32>>> protocol_handler::receive_packet<32>(std::chrono::milliseconds);
template task<result<basic_packet<64>>> protocol_handler::receive_packet<64>(std::chrono::milliseconds);
template task<result<basic_packet<128>>> protocol_handler::receive_packet<128>(std::chrono::milliseconds);
template task<result<basic_packet<256>>> protocol_handler::receive_packet<256>(std::chrono::milliseconds);

} // namespace carbio::internal
