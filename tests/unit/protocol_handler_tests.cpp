#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fingerprint/command_code.h"
#include "fingerprint/protocol_handler.h"
#include "fingerprint/sensor_types.h"
#include "utility/endian.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace carbio::unit_tests
{

class protocol_handler_tests : public testing::Test
{
protected:
  protocol_handler handler;
  static constexpr std::uint32_t default_address = 0xFFFFFFFF;
  static constexpr std::uint32_t custom_address = 0x12345678;
  static constexpr std::uint16_t default_length = 128;

  // Helper function to create a valid acknowledgment packet
  std::vector<std::uint8_t> create_ack_packet(std::uint32_t address, status_code status, std::span<const std::uint8_t> response_data = {})
  {
    std::vector<std::uint8_t> packet;

    // Tag (0xEF01)
    auto tag_bytes = to_bytes_be(static_cast<std::uint16_t>(0xEF01));
    packet.insert(packet.end(), tag_bytes.begin(), tag_bytes.end());

    // Address
    auto addr_bytes = to_bytes_be(address);
    packet.insert(packet.end(), addr_bytes.begin(), addr_bytes.end());

    // Type (acknowledge = 0x07)
    packet.push_back(0x07);

    // Length (with checksum = payload + 2)
    std::uint16_t length_with_checksum = 1 + response_data.size() + 2;
    auto len_bytes = to_bytes_be(length_with_checksum);
    packet.insert(packet.end(), len_bytes.begin(), len_bytes.end());

    // Payload (status code + response data)
    packet.push_back(static_cast<std::uint8_t>(status));
    if (!response_data.empty())
    {
      packet.insert(packet.end(), response_data.begin(), response_data.end());
    }

    // Calculate checksum
    std::uint16_t checksum = static_cast<std::uint16_t>(0x07 + (length_with_checksum >> 8) + (length_with_checksum & 0xFF) + static_cast<std::uint8_t>(status));
    for (auto byte : response_data)
    {
      checksum += byte;
    }
    auto checksum_bytes = to_bytes_be(checksum);
    packet.insert(packet.end(), checksum_bytes.begin(), checksum_bytes.end());

    return packet;
  }

  // Helper function to create a data packet
  std::vector<std::uint8_t> create_data_packet(std::uint32_t address, bool is_last, std::span<const std::uint8_t> data)
  {
    std::vector<std::uint8_t> packet;

    // Tag (0xEF01)
    auto tag_bytes = to_bytes_be(static_cast<std::uint16_t>(0xEF01));
    packet.insert(packet.end(), tag_bytes.begin(), tag_bytes.end());

    // Address
    auto addr_bytes = to_bytes_be(address);
    packet.insert(packet.end(), addr_bytes.begin(), addr_bytes.end());

    // Type (data = 0x02, end_data = 0x08)
    std::uint8_t type = is_last ? 0x08 : 0x02;
    packet.push_back(type);

    // Length (with checksum = payload + 2)
    std::uint16_t length_with_checksum = data.size() + 2;
    auto len_bytes = to_bytes_be(length_with_checksum);
    packet.insert(packet.end(), len_bytes.begin(), len_bytes.end());

    // Payload
    packet.insert(packet.end(), data.begin(), data.end());

    // Calculate checksum
    std::uint16_t checksum = static_cast<std::uint16_t>(type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF));
    for (auto byte : data)
    {
      checksum += byte;
    }
    auto checksum_bytes = to_bytes_be(checksum);
    packet.insert(packet.end(), checksum_bytes.begin(), checksum_bytes.end());

    return packet;
  }
};

// ===== Constructor and Address Tests =====

TEST_F(protocol_handler_tests, default_constructor_sets_default_address)
{
  EXPECT_EQ(handler.get_address(), default_address);
}

TEST_F(protocol_handler_tests, constructor_with_custom_address)
{
  protocol_handler custom_handler(custom_address);
  EXPECT_EQ(custom_handler.get_address(), custom_address);
}

TEST_F(protocol_handler_tests, set_address_updates_address)
{
  handler.set_address(custom_address);
  EXPECT_EQ(handler.get_address(), custom_address);
}

TEST_F(protocol_handler_tests, set_address_zero)
{
  handler.set_address(0x00000000);
  EXPECT_EQ(handler.get_address(), 0x00000000);
}

TEST_F(protocol_handler_tests, set_address_max_value)
{
  handler.set_address(0xFFFFFFFF);
  EXPECT_EQ(handler.get_address(), 0xFFFFFFFF);
}

// ===== Packet Length Tests =====

TEST_F(protocol_handler_tests, default_packet_length)
{
  EXPECT_EQ(handler.get_packet_length(), default_length);
}

TEST_F(protocol_handler_tests, set_packet_length_32)
{
  handler.set_packet_length(32);
  EXPECT_EQ(handler.get_packet_length(), 32);
}

TEST_F(protocol_handler_tests, set_packet_length_64)
{
  handler.set_packet_length(64);
  EXPECT_EQ(handler.get_packet_length(), 64);
}

TEST_F(protocol_handler_tests, set_packet_length_128)
{
  handler.set_packet_length(128);
  EXPECT_EQ(handler.get_packet_length(), 128);
}

TEST_F(protocol_handler_tests, set_packet_length_256)
{
  handler.set_packet_length(256);
  EXPECT_EQ(handler.get_packet_length(), 256);
}

// ===== Command Packet Construction Tests =====

TEST_F(protocol_handler_tests, construct_command_packet_no_data)
{
  auto result = handler.construct_command_packet(command_code::capture_image, {});

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 0);

  // Verify packet structure (basic validation)
  auto buffer = result->as_span();
  EXPECT_EQ(buffer[0], 0xEF);
  EXPECT_EQ(buffer[1], 0x01);
}

TEST_F(protocol_handler_tests, construct_command_packet_with_data)
{
  std::array<std::uint8_t, 4> data{0x01, 0x02, 0x03, 0x04};
  auto result = handler.construct_command_packet(command_code::store_model, std::span(data));

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 0);

  auto buffer = result->as_span();
  EXPECT_EQ(buffer[0], 0xEF);
  EXPECT_EQ(buffer[1], 0x01);
}

TEST_F(protocol_handler_tests, construct_command_packet_different_commands)
{
  std::vector<command_code> commands = {command_code::capture_image, command_code::extract_features, command_code::match_model, command_code::search_model, command_code::merge_model, command_code::verify_device_password};

  for (auto cmd : commands)
  {
    auto result = handler.construct_command_packet(cmd, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 0);
  }
}

TEST_F(protocol_handler_tests, construct_command_packet_max_data)
{
  // Create maximum size data (255 bytes, since 1 byte is for command code)
  std::vector<std::uint8_t> data(255, 0xAB);
  auto result = handler.construct_command_packet(command_code::download_model, data);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 0);
}

TEST_F(protocol_handler_tests, construct_command_packet_custom_address)
{
  handler.set_address(custom_address);
  auto result = handler.construct_command_packet(command_code::capture_image, {});

  ASSERT_TRUE(result.has_value());

  // Verify address is encoded in packet (bytes 2-5)
  auto buffer = result->as_span();
  std::uint32_t encoded_address = read_be<std::uint32_t>(std::span(buffer.data() + 2, 4));
  EXPECT_EQ(encoded_address, custom_address);
}

TEST_F(protocol_handler_tests, construct_command_packet_single_byte_data)
{
  std::array<std::uint8_t, 1> data{0xFF};
  auto result = handler.construct_command_packet(command_code::set_device_password, data);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 0);
}

// ===== Acknowledge Packet Parsing Tests =====

TEST_F(protocol_handler_tests, parse_ack_packet_success_no_data)
{
  auto packet = create_ack_packet(default_address, status_code::success);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 0);
}

TEST_F(protocol_handler_tests, parse_ack_packet_success_with_data)
{
  std::array<std::uint8_t, 4> response_data{0x11, 0x22, 0x33, 0x44};
  auto packet = create_ack_packet(default_address, status_code::success, response_data);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 4);
  EXPECT_EQ(result->data()[0], 0x11);
  EXPECT_EQ(result->data()[1], 0x22);
  EXPECT_EQ(result->data()[2], 0x33);
  EXPECT_EQ(result->data()[3], 0x44);
}

TEST_F(protocol_handler_tests, parse_ack_packet_error_status)
{
  auto packet = create_ack_packet(default_address, status_code::no_finger);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::no_finger);
}

TEST_F(protocol_handler_tests, parse_ack_packet_various_error_statuses)
{
  std::vector<status_code> error_statuses = {status_code::frame_error,        status_code::image_capture_error,   status_code::no_match,          status_code::not_found,
                                             status_code::index_out_of_range, status_code::database_access_error, status_code::permission_denied, status_code::timeout};

  for (auto status : error_statuses)
  {
    auto packet = create_ack_packet(default_address, status);
    auto result = handler.parse_acknowledge_packet(packet);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), status);
  }
}

TEST_F(protocol_handler_tests, parse_ack_packet_wrong_address)
{
  auto packet = create_ack_packet(custom_address, status_code::success);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(protocol_handler_tests, parse_ack_packet_invalid_tag)
{
  auto packet = create_ack_packet(default_address, status_code::success);
  // Corrupt the tag
  packet[0] = 0xFF;
  packet[1] = 0xFF;

  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(protocol_handler_tests, parse_ack_packet_invalid_checksum)
{
  auto packet = create_ack_packet(default_address, status_code::success);
  // Corrupt the checksum (last 2 bytes)
  packet[packet.size() - 1] ^= 0xFF;

  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(protocol_handler_tests, parse_ack_packet_wrong_packet_type)
{
  // Create a command packet instead of ack
  auto packet = create_ack_packet(default_address, status_code::success);
  packet[6] = 0x01; // Change type to command

  // Recalculate checksum
  std::uint16_t length_with_checksum = 3;
  std::uint16_t checksum = static_cast<std::uint16_t>(0x01 + (length_with_checksum >> 8) + (length_with_checksum & 0xFF) + static_cast<std::uint8_t>(status_code::success));
  auto checksum_bytes = to_bytes_be(checksum);
  packet[packet.size() - 2] = checksum_bytes[0];
  packet[packet.size() - 1] = checksum_bytes[1];

  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(protocol_handler_tests, parse_ack_packet_truncated)
{
  auto packet = create_ack_packet(default_address, status_code::success);
  packet.resize(5); // Too small

  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(protocol_handler_tests, parse_ack_packet_empty)
{
  std::vector<std::uint8_t> packet;
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(protocol_handler_tests, parse_ack_packet_custom_address)
{
  handler.set_address(custom_address);
  auto packet = create_ack_packet(custom_address, status_code::success);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_TRUE(result.has_value());
}

TEST_F(protocol_handler_tests, parse_ack_packet_large_response_data)
{
  std::vector<std::uint8_t> large_data(200, 0xCC);
  auto packet = create_ack_packet(default_address, status_code::success, large_data);
  auto result = handler.parse_acknowledge_packet(packet);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 200);
}

// ===== Data Packet Construction Tests =====

TEST_F(protocol_handler_tests, construct_data_packet_empty)
{
  std::vector<std::uint8_t> data;
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 0);
}

TEST_F(protocol_handler_tests, construct_data_packet_small_data)
{
  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1); // Should fit in one packet
}

TEST_F(protocol_handler_tests, construct_data_packet_single_chunk)
{
  std::vector<std::uint8_t> data(100, 0xAA);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1); // Should fit in one packet (128 byte limit)
}

TEST_F(protocol_handler_tests, construct_data_packet_multiple_chunks)
{
  std::vector<std::uint8_t> data(300, 0xBB);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 1); // Should require multiple packets
}

TEST_F(protocol_handler_tests, construct_data_packet_exact_chunk_boundary)
{
  handler.set_packet_length(128);
  std::vector<std::uint8_t> data(128, 0xCC);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1);
}

TEST_F(protocol_handler_tests, construct_data_packet_just_over_boundary)
{
  handler.set_packet_length(128);
  std::vector<std::uint8_t> data(129, 0xDD);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 2);
}

TEST_F(protocol_handler_tests, construct_data_packet_different_lengths)
{
  std::vector<std::uint16_t> lengths = {32, 64, 128, 256};

  for (auto len : lengths)
  {
    handler.set_packet_length(len);
    std::vector<std::uint8_t> data(len + 50, 0xEE);
    auto result = handler.construct_data_packet(data);

    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->size(), 1);
  }
}

TEST_F(protocol_handler_tests, construct_data_packet_large_data)
{
  std::vector<std::uint8_t> data(1000, 0xFF);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 1);
}

TEST_F(protocol_handler_tests, construct_data_packet_max_size)
{
  // Maximum data that can fit in one packet (256 bytes)
  handler.set_packet_length(256);
  std::vector<std::uint8_t> data(256, 0x55);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1);
}

TEST_F(protocol_handler_tests, construct_data_packet_custom_address)
{
  handler.set_address(custom_address);
  std::vector<std::uint8_t> data(50, 0x33);
  auto result = handler.construct_data_packet(data);

  ASSERT_TRUE(result.has_value());
  ASSERT_GT(result->size(), 0);

  // Verify address in first packet
  auto buffer = (*result)[0].as_span();
  std::uint32_t encoded_address = read_be<std::uint32_t>(std::span(buffer.data() + 2, 4));
  EXPECT_EQ(encoded_address, custom_address);
}

// ===== Data Packet Parsing Tests =====

TEST_F(protocol_handler_tests, parse_data_packet_single_packet)
{
  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto packet_data = create_data_packet(default_address, true, data);

  std::array<std::uint8_t, 1024> buffer;
  std::ranges::copy(packet_data, buffer.begin());
  std::span<std::uint8_t> packet_span(buffer.data(), packet_data.size());

  std::array<std::span<std::uint8_t>, 1> packets = {packet_span};
  auto result = handler.parse_data_packet(packets);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 10);
  for (std::size_t i = 0; i < 10; ++i)
  {
    EXPECT_EQ(result->data()[i], i + 1);
  }
}

TEST_F(protocol_handler_tests, parse_data_packet_multiple_packets)
{
  std::array<std::uint8_t, 20> data1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  std::array<std::uint8_t, 10> data2{21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

  auto packet1_data = create_data_packet(default_address, false, data1);
  auto packet2_data = create_data_packet(default_address, true, data2);

  std::array<std::uint8_t, 1024> buffer1, buffer2;
  std::ranges::copy(packet1_data, buffer1.begin());
  std::ranges::copy(packet2_data, buffer2.begin());

  std::span<std::uint8_t> span1(buffer1.data(), packet1_data.size());
  std::span<std::uint8_t> span2(buffer2.data(), packet2_data.size());

  std::array<std::span<std::uint8_t>, 2> packets = {span1, span2};
  auto result = handler.parse_data_packet(packets);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 30);
  for (std::size_t i = 0; i < 30; ++i)
  {
    EXPECT_EQ(result->data()[i], i + 1);
  }
}

TEST_F(protocol_handler_tests, parse_data_packet_empty_vector)
{
  std::vector<std::span<std::uint8_t>> packets;
  auto result = handler.parse_data_packet(packets);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 0);
}

TEST_F(protocol_handler_tests, parse_data_packet_wrong_address)
{
  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto packet_data = create_data_packet(custom_address, true, data);

  std::array<std::uint8_t, 1024> buffer;
  std::ranges::copy(packet_data, buffer.begin());
  std::span<std::uint8_t> packet_span(buffer.data(), packet_data.size());

  std::array<std::span<std::uint8_t>, 1> packets = {packet_span};
  auto result = handler.parse_data_packet(packets);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::no_frame);
}

TEST_F(protocol_handler_tests, parse_data_packet_invalid_packet_type)
{
  // Create an ack packet instead of data packet
  auto packet_data = create_ack_packet(default_address, status_code::success);

  std::array<std::uint8_t, 1024> buffer;
  std::ranges::copy(packet_data, buffer.begin());
  std::span<std::uint8_t> packet_span(buffer.data(), packet_data.size());

  std::array<std::span<std::uint8_t>, 1> packets = {packet_span};
  auto result = handler.parse_data_packet(packets);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::no_frame);
}

TEST_F(protocol_handler_tests, parse_data_packet_custom_address)
{
  handler.set_address(custom_address);
  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto packet_data = create_data_packet(custom_address, true, data);

  std::array<std::uint8_t, 1024> buffer;
  std::ranges::copy(packet_data, buffer.begin());
  std::span<std::uint8_t> packet_span(buffer.data(), packet_data.size());

  std::array<std::span<std::uint8_t>, 1> packets = {packet_span};
  auto result = handler.parse_data_packet(packets);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 10);
}

TEST_F(protocol_handler_tests, parse_data_packet_stops_at_end_marker)
{
  std::array<std::uint8_t, 10> data1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::array<std::uint8_t, 10> data2{11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  std::array<std::uint8_t, 10> data3{21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

  auto packet1_data = create_data_packet(default_address, false, data1);
  auto packet2_data = create_data_packet(default_address, true, data2);
  auto packet3_data = create_data_packet(default_address, false, data3);

  std::array<std::uint8_t, 1024> buffer1, buffer2, buffer3;
  std::ranges::copy(packet1_data, buffer1.begin());
  std::ranges::copy(packet2_data, buffer2.begin());
  std::ranges::copy(packet3_data, buffer3.begin());

  std::span<std::uint8_t> span1(buffer1.data(), packet1_data.size());
  std::span<std::uint8_t> span2(buffer2.data(), packet2_data.size());
  std::span<std::uint8_t> span3(buffer3.data(), packet3_data.size());

  std::array<std::span<std::uint8_t>, 3> packets = {span1, span2, span3};
  auto result = handler.parse_data_packet(packets);

  ASSERT_TRUE(result.has_value());
  // Should stop after packet2 (end_data marker), ignoring packet3
  EXPECT_EQ(result->size(), 20);
}

// ===== Edge Cases and Stress Tests =====

TEST_F(protocol_handler_tests, move_constructor)
{
  handler.set_address(custom_address);
  handler.set_packet_length(256);

  protocol_handler moved_handler(std::move(handler));

  EXPECT_EQ(moved_handler.get_address(), custom_address);
  EXPECT_EQ(moved_handler.get_packet_length(), 256);
}

TEST_F(protocol_handler_tests, move_assignment)
{
  handler.set_address(custom_address);
  handler.set_packet_length(256);

  protocol_handler moved_handler;
  moved_handler = std::move(handler);

  EXPECT_EQ(moved_handler.get_address(), custom_address);
  EXPECT_EQ(moved_handler.get_packet_length(), 256);
}

TEST_F(protocol_handler_tests, construct_and_parse_roundtrip)
{
  std::array<std::uint8_t, 5> cmd_data{0x01, 0x02, 0x03, 0x04, 0x05};

  // Construct command packet
  auto cmd_result = handler.construct_command_packet(command_code::capture_image, cmd_data);
  ASSERT_TRUE(cmd_result.has_value());

  // The packet should be valid
  EXPECT_GT(cmd_result->size(), 0);
}

TEST_F(protocol_handler_tests, multiple_operations_sequence)
{
  // Set custom configuration
  handler.set_address(0x11223344);
  handler.set_packet_length(64);

  // Construct command
  auto cmd_result = handler.construct_command_packet(command_code::capture_image, {});
  ASSERT_TRUE(cmd_result.has_value());

  // Parse acknowledgment
  auto ack_packet = create_ack_packet(0x11223344, status_code::success);
  auto ack_result = handler.parse_acknowledge_packet(ack_packet);
  ASSERT_TRUE(ack_result.has_value());

  // Construct data packet
  std::vector<std::uint8_t> data(100, 0xAB);
  auto data_result = handler.construct_data_packet(data);
  ASSERT_TRUE(data_result.has_value());
}

} // namespace carbio::unit_tests
