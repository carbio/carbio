#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fingerprint/packet.h"
#include "fingerprint/sensor_types.h"
#include "utility/endian.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace carbio::unit_tests
{
class packet_tests : public testing::Test
{
protected:
  packet pkt;
  static constexpr std::uint32_t test_address = 0xFFFFFFFF;
  static constexpr std::uint32_t custom_address = 0x12345678;

  // Helper to create a valid encoded packet
  std::vector<std::uint8_t> create_encoded_packet(std::uint16_t tag, std::uint32_t address, std::uint8_t type, std::uint16_t data_length, std::span<const std::uint8_t> data)
  {
    std::vector<std::uint8_t> buffer;

    // Tag
    auto tag_bytes = to_bytes_be(tag);
    buffer.insert(buffer.end(), tag_bytes.begin(), tag_bytes.end());

    // Address
    auto addr_bytes = to_bytes_be(address);
    buffer.insert(buffer.end(), addr_bytes.begin(), addr_bytes.end());

    // Type
    buffer.push_back(type);

    // Length (with checksum)
    std::uint16_t length_with_checksum = data_length + 2;
    auto len_bytes = to_bytes_be(length_with_checksum);
    buffer.insert(buffer.end(), len_bytes.begin(), len_bytes.end());

    // Data
    if (data_length > 0)
    {
      buffer.insert(buffer.end(), data.begin(), data.begin() + data_length);
    }

    // Calculate checksum
    std::uint16_t checksum = static_cast<std::uint16_t>(type + (length_with_checksum >> 8) + (length_with_checksum & 0xFF));
    for (std::size_t i = 0; i < data_length; ++i)
    {
      checksum += data[i];
    }
    auto checksum_bytes = to_bytes_be(checksum);
    buffer.insert(buffer.end(), checksum_bytes.begin(), checksum_bytes.end());

    return buffer;
  }
};

// ===== Constructor Tests =====

TEST_F(packet_tests, default_constructor)
{
  EXPECT_EQ(pkt.tag, packet::builtin_tag);
  EXPECT_EQ(pkt.address, 0xFFFFFFFF);
  EXPECT_EQ(pkt.type, 0x01);
  EXPECT_EQ(pkt.length, 0);
}

TEST_F(packet_tests, default_values)
{
  EXPECT_EQ(pkt.tag, 0xEF01);
  EXPECT_EQ(pkt.address, test_address);
}

// ===== Move Semantics Tests =====

TEST_F(packet_tests, move_constructor)
{
  pkt.address = custom_address;
  pkt.type = 0x07;
  pkt.length = 5;
  pkt.data[0] = 0xAA;
  pkt.data[1] = 0xBB;

  packet moved_pkt(std::move(pkt));

  EXPECT_EQ(moved_pkt.address, custom_address);
  EXPECT_EQ(moved_pkt.type, 0x07);
  EXPECT_EQ(moved_pkt.length, 5);
  EXPECT_EQ(moved_pkt.data[0], 0xAA);
  EXPECT_EQ(moved_pkt.data[1], 0xBB);

  // Source should be cleared
  EXPECT_EQ(pkt.length, 0);
}

TEST_F(packet_tests, move_assignment)
{
  pkt.address = custom_address;
  pkt.type = 0x07;
  pkt.length = 5;
  pkt.data[0] = 0xAA;
  pkt.data[1] = 0xBB;

  packet moved_pkt;
  moved_pkt = std::move(pkt);

  EXPECT_EQ(moved_pkt.address, custom_address);
  EXPECT_EQ(moved_pkt.type, 0x07);
  EXPECT_EQ(moved_pkt.length, 5);
  EXPECT_EQ(moved_pkt.data[0], 0xAA);
  EXPECT_EQ(moved_pkt.data[1], 0xBB);

  // Source should be cleared
  EXPECT_EQ(pkt.length, 0);
}

TEST_F(packet_tests, move_assignment_self)
{
  pkt.address = custom_address;
  pkt.type = 0x07;
  pkt.length = 5;

  pkt = std::move(pkt);

  // Self-assignment should be safe
  EXPECT_EQ(pkt.address, custom_address);
  EXPECT_EQ(pkt.type, 0x07);
  EXPECT_EQ(pkt.length, 5);
}

// ===== Secure Clear Tests =====

TEST_F(packet_tests, secure_clear)
{
  pkt.length = 10;
  for (std::size_t i = 0; i < 10; ++i)
  {
    pkt.data[i] = static_cast<std::uint8_t>(i);
  }

  pkt.secure_clear();

  EXPECT_EQ(pkt.length, 0);
  for (std::size_t i = 0; i < 10; ++i)
  {
    EXPECT_EQ(pkt.data[i], 0);
  }
}

TEST_F(packet_tests, secure_clear_full_buffer)
{
  pkt.length = packet::max_data_size;
  std::ranges::fill(pkt.data, 0xFF);

  pkt.secure_clear();

  EXPECT_EQ(pkt.length, 0);
  for (auto byte : pkt.data)
  {
    EXPECT_EQ(byte, 0);
  }
}

TEST_F(packet_tests, destructor_clears_data)
{
  {
    packet temp_pkt;
    temp_pkt.length = 5;
    temp_pkt.data[0] = 0xAA;
    temp_pkt.data[1] = 0xBB;
    // Destructor will be called here
  }
  // If we could inspect the memory after destruction, it should be zeroed
  // This test just verifies the destructor doesn't crash
}

// ===== Encode Tests =====

TEST_F(packet_tests, encode_empty_packet)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::command);
  pkt.length = 0;

  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto result = pkt.encode(buffer);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, packet::max_header_size + 2); // Header + checksum
}

TEST_F(packet_tests, encode_with_data)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::command);
  pkt.length = 5;
  pkt.data[0] = 0x01;
  pkt.data[1] = 0x02;
  pkt.data[2] = 0x03;
  pkt.data[3] = 0x04;
  pkt.data[4] = 0x05;

  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto result = pkt.encode(buffer);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, packet::max_header_size + 5 + 2);

  // Verify tag
  EXPECT_EQ(buffer[0], 0xEF);
  EXPECT_EQ(buffer[1], 0x01);

  // Verify address (big-endian)
  std::uint32_t encoded_addr = read_be<std::uint32_t>(std::span(buffer.data() + 2, 4));
  EXPECT_EQ(encoded_addr, test_address);

  // Verify type
  EXPECT_EQ(buffer[6], static_cast<std::uint8_t>(packet_id::command));

  // Verify length (big-endian, includes checksum)
  std::uint16_t encoded_len = read_be<std::uint16_t>(std::span(buffer.data() + 7, 2));
  EXPECT_EQ(encoded_len, 7); // 5 + 2 for checksum

  // Verify data
  EXPECT_EQ(buffer[9], 0x01);
  EXPECT_EQ(buffer[10], 0x02);
  EXPECT_EQ(buffer[11], 0x03);
  EXPECT_EQ(buffer[12], 0x04);
  EXPECT_EQ(buffer[13], 0x05);
}

TEST_F(packet_tests, encode_buffer_too_small)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::command);
  pkt.length = 10;

  std::array<std::uint8_t, 5> small_buffer{};
  auto result = pkt.encode(small_buffer);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, encode_max_data_size)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::data);
  pkt.length = packet::max_data_size;
  std::ranges::fill(pkt.data, 0xAA);

  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto result = pkt.encode(buffer);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, packet::max_header_size + packet::max_data_size + 2);
}

TEST_F(packet_tests, encode_different_packet_types)
{
  std::vector<packet_id> types = {packet_id::command, packet_id::data, packet_id::acknowledge, packet_id::end_data};

  for (auto type : types)
  {
    pkt.address = test_address;
    pkt.type = static_cast<std::uint8_t>(type);
    pkt.length = 5;
    std::ranges::fill_n(pkt.data.begin(), 5, 0x11);

    std::array<std::uint8_t, packet::max_packet_size> buffer{};
    auto result = pkt.encode(buffer);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(buffer[6], static_cast<std::uint8_t>(type));
  }
}

TEST_F(packet_tests, encode_custom_address)
{
  pkt.address = custom_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::command);
  pkt.length = 0;

  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto result = pkt.encode(buffer);

  ASSERT_TRUE(result.has_value());

  std::uint32_t encoded_addr = read_be<std::uint32_t>(std::span(buffer.data() + 2, 4));
  EXPECT_EQ(encoded_addr, custom_address);
}

TEST_F(packet_tests, encode_checksum_calculation)
{
  pkt.address = test_address;
  pkt.type = 0x07; // acknowledge
  pkt.length = 3;
  pkt.data[0] = 0x00; // status success
  pkt.data[1] = 0x10;
  pkt.data[2] = 0x20;

  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto result = pkt.encode(buffer);

  ASSERT_TRUE(result.has_value());

  // Calculate expected checksum
  std::uint16_t length_with_checksum = 5;
  std::uint16_t expected_checksum = static_cast<std::uint16_t>(0x07 + (length_with_checksum >> 8) + (length_with_checksum & 0xFF) + 0x00 + 0x10 + 0x20);

  // Extract checksum from buffer
  std::size_t checksum_offset = packet::max_header_size + 3;
  std::uint16_t encoded_checksum = read_be<std::uint16_t>(std::span(buffer.data() + checksum_offset, 2));

  EXPECT_EQ(encoded_checksum, expected_checksum);
}

// ===== Decode Tests =====

TEST_F(packet_tests, decode_valid_packet)
{
  std::array<std::uint8_t, 5> data{0x01, 0x02, 0x03, 0x04, 0x05};
  auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(packet_id::command), 5, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(pkt.tag, packet::builtin_tag);
  EXPECT_EQ(pkt.address, test_address);
  EXPECT_EQ(pkt.type, static_cast<std::uint8_t>(packet_id::command));
  EXPECT_EQ(pkt.length, 5);
  EXPECT_EQ(pkt.data[0], 0x01);
  EXPECT_EQ(pkt.data[1], 0x02);
  EXPECT_EQ(pkt.data[2], 0x03);
  EXPECT_EQ(pkt.data[3], 0x04);
  EXPECT_EQ(pkt.data[4], 0x05);
}

TEST_F(packet_tests, decode_empty_packet)
{
  std::array<std::uint8_t, 0> data{};
  auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(packet_id::command), 0, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(pkt.length, 0);
}

TEST_F(packet_tests, decode_buffer_too_small)
{
  std::array<std::uint8_t, 5> small_buffer{0xEF, 0x01, 0xFF, 0xFF, 0xFF};

  auto result = pkt.decode(small_buffer, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(packet_tests, decode_invalid_tag)
{
  std::array<std::uint8_t, 5> data{0x01, 0x02, 0x03, 0x04, 0x05};
  auto encoded = create_encoded_packet(0xAAAA, test_address, static_cast<std::uint8_t>(packet_id::command), 5, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, decode_wrong_address)
{
  std::array<std::uint8_t, 5> data{0x01, 0x02, 0x03, 0x04, 0x05};
  auto encoded = create_encoded_packet(packet::builtin_tag, custom_address, static_cast<std::uint8_t>(packet_id::command), 5, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, decode_invalid_checksum)
{
  std::array<std::uint8_t, 5> data{0x01, 0x02, 0x03, 0x04, 0x05};
  auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(packet_id::command), 5, data);

  // Corrupt checksum
  encoded[encoded.size() - 1] ^= 0xFF;

  auto result = pkt.decode(encoded, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, decode_length_too_large)
{
  std::vector<std::uint8_t> buffer;

  // Tag
  buffer.push_back(0xEF);
  buffer.push_back(0x01);

  // Address
  auto addr_bytes = to_bytes_be(test_address);
  buffer.insert(buffer.end(), addr_bytes.begin(), addr_bytes.end());

  // Type
  buffer.push_back(0x01);

  // Length (with checksum) - set to exceed max_data_size
  std::uint16_t invalid_length = packet::max_data_size + 10;
  auto len_bytes = to_bytes_be(invalid_length);
  buffer.insert(buffer.end(), len_bytes.begin(), len_bytes.end());

  auto result = pkt.decode(buffer, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, decode_length_less_than_minimum)
{
  std::vector<std::uint8_t> buffer;

  // Tag
  buffer.push_back(0xEF);
  buffer.push_back(0x01);

  // Address
  auto addr_bytes = to_bytes_be(test_address);
  buffer.insert(buffer.end(), addr_bytes.begin(), addr_bytes.end());

  // Type
  buffer.push_back(0x01);

  // Length (with checksum) - set to less than 2 (minimum for checksum)
  std::uint16_t invalid_length = 1;
  auto len_bytes = to_bytes_be(invalid_length);
  buffer.insert(buffer.end(), len_bytes.begin(), len_bytes.end());

  auto result = pkt.decode(buffer, test_address);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(packet_tests, decode_different_packet_types)
{
  std::vector<packet_id> types = {packet_id::command, packet_id::data, packet_id::acknowledge, packet_id::end_data};

  for (auto type : types)
  {
    std::array<std::uint8_t, 5> data{0x11, 0x22, 0x33, 0x44, 0x55};
    auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(type), 5, data);

    packet test_pkt;
    auto result = test_pkt.decode(encoded, test_address);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(test_pkt.type, static_cast<std::uint8_t>(type));
  }
}

TEST_F(packet_tests, decode_custom_address)
{
  std::array<std::uint8_t, 5> data{0x01, 0x02, 0x03, 0x04, 0x05};
  auto encoded = create_encoded_packet(packet::builtin_tag, custom_address, static_cast<std::uint8_t>(packet_id::command), 5, data);

  auto result = pkt.decode(encoded, custom_address);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(pkt.address, custom_address);
}

TEST_F(packet_tests, decode_max_data_size)
{
  std::vector<std::uint8_t> data(packet::max_data_size, 0xBB);
  auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(packet_id::data), packet::max_data_size, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(pkt.length, packet::max_data_size);
  for (std::size_t i = 0; i < packet::max_data_size; ++i)
  {
    EXPECT_EQ(pkt.data[i], 0xBB);
  }
}

TEST_F(packet_tests, decode_returns_correct_size)
{
  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto encoded = create_encoded_packet(packet::builtin_tag, test_address, static_cast<std::uint8_t>(packet_id::command), 10, data);

  auto result = pkt.decode(encoded, test_address);

  ASSERT_TRUE(result.has_value());
  // Should return total packet size: header(9) + data(10) + checksum(2) = 21
  EXPECT_EQ(*result, packet::max_header_size + 10 + 2);
}

// ===== Encode/Decode Roundtrip Tests =====

TEST_F(packet_tests, encode_decode_roundtrip)
{
  pkt.address = custom_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::acknowledge);
  pkt.length = 8;
  for (std::size_t i = 0; i < 8; ++i)
  {
    pkt.data[i] = static_cast<std::uint8_t>(i * 2);
  }

  // Encode
  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto encode_result = pkt.encode(buffer);
  ASSERT_TRUE(encode_result.has_value());

  // Decode
  packet decoded_pkt;
  auto decode_result = decoded_pkt.decode(std::span(buffer.data(), *encode_result), custom_address);
  ASSERT_TRUE(decode_result.has_value());

  // Verify
  EXPECT_EQ(decoded_pkt.tag, pkt.tag);
  EXPECT_EQ(decoded_pkt.address, pkt.address);
  EXPECT_EQ(decoded_pkt.type, pkt.type);
  EXPECT_EQ(decoded_pkt.length, pkt.length);
  for (std::size_t i = 0; i < pkt.length; ++i)
  {
    EXPECT_EQ(decoded_pkt.data[i], pkt.data[i]);
  }
}

TEST_F(packet_tests, encode_decode_roundtrip_empty)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::command);
  pkt.length = 0;

  // Encode
  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto encode_result = pkt.encode(buffer);
  ASSERT_TRUE(encode_result.has_value());

  // Decode
  packet decoded_pkt;
  auto decode_result = decoded_pkt.decode(std::span(buffer.data(), *encode_result), test_address);
  ASSERT_TRUE(decode_result.has_value());

  // Verify
  EXPECT_EQ(decoded_pkt.length, 0);
}

TEST_F(packet_tests, encode_decode_roundtrip_max_size)
{
  pkt.address = test_address;
  pkt.type = static_cast<std::uint8_t>(packet_id::data);
  pkt.length = packet::max_data_size;
  std::ranges::fill(pkt.data, 0xCC);

  // Encode
  std::array<std::uint8_t, packet::max_packet_size> buffer{};
  auto encode_result = pkt.encode(buffer);
  ASSERT_TRUE(encode_result.has_value());

  // Decode
  packet decoded_pkt;
  auto decode_result = decoded_pkt.decode(std::span(buffer.data(), *encode_result), test_address);
  ASSERT_TRUE(decode_result.has_value());

  // Verify
  EXPECT_EQ(decoded_pkt.length, packet::max_data_size);
  for (std::size_t i = 0; i < packet::max_data_size; ++i)
  {
    EXPECT_EQ(decoded_pkt.data[i], 0xCC);
  }
}

// ===== Constants Tests =====

TEST_F(packet_tests, builtin_tag_value)
{
  EXPECT_EQ(packet::builtin_tag, 0xEF01);
}

TEST_F(packet_tests, max_header_size)
{
  EXPECT_EQ(packet::max_header_size, 9);
}

TEST_F(packet_tests, max_data_size)
{
  EXPECT_EQ(packet::max_data_size, 256);
}

TEST_F(packet_tests, max_packet_size)
{
  EXPECT_EQ(packet::max_packet_size, packet::max_header_size + packet::max_data_size + 2);
}

} // namespace carbio::unit_tests
