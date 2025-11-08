#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utility/endian.h"

#include <array>
#include <cstdint>
#include <span>

namespace carbio::unit_tests
{

class endian_tests : public testing::Test
{
protected:
};

// ===== Byteswap Tests =====

TEST_F(endian_tests, byteswap_uint8)
{
  std::uint8_t value = 0xAB;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0xAB); // no-op for single byte
}

TEST_F(endian_tests, byteswap_uint16)
{
  std::uint16_t value = 0x1234;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0x3412);
}

TEST_F(endian_tests, byteswap_uint32)
{
  std::uint32_t value = 0x12345678;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0x78563412);
}

TEST_F(endian_tests, byteswap_uint64)
{
  std::uint64_t value = 0x123456789ABCDEF0ULL;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0xF0DEBC9A78563412ULL);
}

TEST_F(endian_tests, byteswap_uint16_zero)
{
  std::uint16_t value = 0x0000;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0x0000);
}

TEST_F(endian_tests, byteswap_uint16_max)
{
  std::uint16_t value = 0xFFFF;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0xFFFF);
}

TEST_F(endian_tests, byteswap_uint32_zero)
{
  std::uint32_t value = 0x00000000;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0x00000000);
}

TEST_F(endian_tests, byteswap_uint32_max)
{
  std::uint32_t value = 0xFFFFFFFF;
  auto result = byteswap(value);

  EXPECT_EQ(result, 0xFFFFFFFF);
}

TEST_F(endian_tests, byteswap_twice_returns_original)
{
  std::uint32_t value = 0x12345678;
  auto swapped = byteswap(value);
  auto restored = byteswap(swapped);

  EXPECT_EQ(restored, value);
}

// ===== Read Big-Endian Tests =====

TEST_F(endian_tests, read_be_uint8)
{
  std::array<std::uint8_t, 1> buffer{0xAB};
  auto result = read_be<std::uint8_t>(buffer);

  EXPECT_EQ(result, 0xAB);
}

TEST_F(endian_tests, read_be_uint16)
{
  std::array<std::uint8_t, 2> buffer{0x12, 0x34};
  auto result = read_be<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0x1234);
}

TEST_F(endian_tests, read_be_uint32)
{
  std::array<std::uint8_t, 4> buffer{0x12, 0x34, 0x56, 0x78};
  auto result = read_be<std::uint32_t>(buffer);

  EXPECT_EQ(result, 0x12345678);
}

TEST_F(endian_tests, read_be_uint64)
{
  std::array<std::uint8_t, 8> buffer{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
  auto result = read_be<std::uint64_t>(buffer);

  EXPECT_EQ(result, 0x123456789ABCDEF0ULL);
}

TEST_F(endian_tests, read_be_uint16_zero)
{
  std::array<std::uint8_t, 2> buffer{0x00, 0x00};
  auto result = read_be<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0x0000);
}

TEST_F(endian_tests, read_be_uint16_max)
{
  std::array<std::uint8_t, 2> buffer{0xFF, 0xFF};
  auto result = read_be<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0xFFFF);
}

TEST_F(endian_tests, read_be_from_larger_buffer)
{
  std::array<std::uint8_t, 10> buffer{0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // Read from offset 1
  auto result = read_be<std::uint16_t>(std::span(buffer).subspan(1, 2));

  EXPECT_EQ(result, 0x1234);
}

// ===== Read Little-Endian Tests =====

TEST_F(endian_tests, read_le_uint8)
{
  std::array<std::uint8_t, 1> buffer{0xAB};
  auto result = read_le<std::uint8_t>(buffer);

  EXPECT_EQ(result, 0xAB);
}

TEST_F(endian_tests, read_le_uint16)
{
  std::array<std::uint8_t, 2> buffer{0x34, 0x12};
  auto result = read_le<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0x1234);
}

TEST_F(endian_tests, read_le_uint32)
{
  std::array<std::uint8_t, 4> buffer{0x78, 0x56, 0x34, 0x12};
  auto result = read_le<std::uint32_t>(buffer);

  EXPECT_EQ(result, 0x12345678);
}

TEST_F(endian_tests, read_le_uint64)
{
  std::array<std::uint8_t, 8> buffer{0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};
  auto result = read_le<std::uint64_t>(buffer);

  EXPECT_EQ(result, 0x123456789ABCDEF0ULL);
}

TEST_F(endian_tests, read_le_uint16_zero)
{
  std::array<std::uint8_t, 2> buffer{0x00, 0x00};
  auto result = read_le<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0x0000);
}

TEST_F(endian_tests, read_le_uint16_max)
{
  std::array<std::uint8_t, 2> buffer{0xFF, 0xFF};
  auto result = read_le<std::uint16_t>(buffer);

  EXPECT_EQ(result, 0xFFFF);
}

// ===== Write Big-Endian Tests =====

TEST_F(endian_tests, write_be_uint8)
{
  std::array<std::uint8_t, 1> buffer{};
  write_be<std::uint8_t>(buffer, 0xAB);

  EXPECT_EQ(buffer[0], 0xAB);
}

TEST_F(endian_tests, write_be_uint16)
{
  std::array<std::uint8_t, 2> buffer{};
  write_be<std::uint16_t>(buffer, 0x1234);

  EXPECT_EQ(buffer[0], 0x12);
  EXPECT_EQ(buffer[1], 0x34);
}

TEST_F(endian_tests, write_be_uint32)
{
  std::array<std::uint8_t, 4> buffer{};
  write_be<std::uint32_t>(buffer, 0x12345678);

  EXPECT_EQ(buffer[0], 0x12);
  EXPECT_EQ(buffer[1], 0x34);
  EXPECT_EQ(buffer[2], 0x56);
  EXPECT_EQ(buffer[3], 0x78);
}

TEST_F(endian_tests, write_be_uint64)
{
  std::array<std::uint8_t, 8> buffer{};
  write_be<std::uint64_t>(buffer, 0x123456789ABCDEF0ULL);

  EXPECT_EQ(buffer[0], 0x12);
  EXPECT_EQ(buffer[1], 0x34);
  EXPECT_EQ(buffer[2], 0x56);
  EXPECT_EQ(buffer[3], 0x78);
  EXPECT_EQ(buffer[4], 0x9A);
  EXPECT_EQ(buffer[5], 0xBC);
  EXPECT_EQ(buffer[6], 0xDE);
  EXPECT_EQ(buffer[7], 0xF0);
}

TEST_F(endian_tests, write_be_uint16_zero)
{
  std::array<std::uint8_t, 2> buffer{0xFF, 0xFF};
  write_be<std::uint16_t>(buffer, 0x0000);

  EXPECT_EQ(buffer[0], 0x00);
  EXPECT_EQ(buffer[1], 0x00);
}

TEST_F(endian_tests, write_be_uint16_max)
{
  std::array<std::uint8_t, 2> buffer{0x00, 0x00};
  write_be<std::uint16_t>(buffer, 0xFFFF);

  EXPECT_EQ(buffer[0], 0xFF);
  EXPECT_EQ(buffer[1], 0xFF);
}

TEST_F(endian_tests, write_be_to_larger_buffer)
{
  std::array<std::uint8_t, 10> buffer{};
  std::ranges::fill(buffer, 0x00);

  // Write to offset 2
  write_be<std::uint16_t>(std::span(buffer).subspan(2, 2), 0x1234);

  EXPECT_EQ(buffer[0], 0x00);
  EXPECT_EQ(buffer[1], 0x00);
  EXPECT_EQ(buffer[2], 0x12);
  EXPECT_EQ(buffer[3], 0x34);
  EXPECT_EQ(buffer[4], 0x00);
}

// ===== Write Little-Endian Tests =====

TEST_F(endian_tests, write_le_uint8)
{
  std::array<std::uint8_t, 1> buffer{};
  write_le<std::uint8_t>(buffer, 0xAB);

  EXPECT_EQ(buffer[0], 0xAB);
}

TEST_F(endian_tests, write_le_uint16)
{
  std::array<std::uint8_t, 2> buffer{};
  write_le<std::uint16_t>(buffer, 0x1234);

  EXPECT_EQ(buffer[0], 0x34);
  EXPECT_EQ(buffer[1], 0x12);
}

TEST_F(endian_tests, write_le_uint32)
{
  std::array<std::uint8_t, 4> buffer{};
  write_le<std::uint32_t>(buffer, 0x12345678);

  EXPECT_EQ(buffer[0], 0x78);
  EXPECT_EQ(buffer[1], 0x56);
  EXPECT_EQ(buffer[2], 0x34);
  EXPECT_EQ(buffer[3], 0x12);
}

TEST_F(endian_tests, write_le_uint64)
{
  std::array<std::uint8_t, 8> buffer{};
  write_le<std::uint64_t>(buffer, 0x123456789ABCDEF0ULL);

  EXPECT_EQ(buffer[0], 0xF0);
  EXPECT_EQ(buffer[1], 0xDE);
  EXPECT_EQ(buffer[2], 0xBC);
  EXPECT_EQ(buffer[3], 0x9A);
  EXPECT_EQ(buffer[4], 0x78);
  EXPECT_EQ(buffer[5], 0x56);
  EXPECT_EQ(buffer[6], 0x34);
  EXPECT_EQ(buffer[7], 0x12);
}

TEST_F(endian_tests, write_le_uint16_zero)
{
  std::array<std::uint8_t, 2> buffer{0xFF, 0xFF};
  write_le<std::uint16_t>(buffer, 0x0000);

  EXPECT_EQ(buffer[0], 0x00);
  EXPECT_EQ(buffer[1], 0x00);
}

TEST_F(endian_tests, write_le_uint16_max)
{
  std::array<std::uint8_t, 2> buffer{0x00, 0x00};
  write_le<std::uint16_t>(buffer, 0xFFFF);

  EXPECT_EQ(buffer[0], 0xFF);
  EXPECT_EQ(buffer[1], 0xFF);
}

// ===== to_bytes_be Tests =====

TEST_F(endian_tests, to_bytes_be_uint8)
{
  auto result = to_bytes_be<std::uint8_t>(0xAB);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0xAB);
}

TEST_F(endian_tests, to_bytes_be_uint16)
{
  auto result = to_bytes_be<std::uint16_t>(0x1234);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x12);
  EXPECT_EQ(result[1], 0x34);
}

TEST_F(endian_tests, to_bytes_be_uint32)
{
  auto result = to_bytes_be<std::uint32_t>(0x12345678);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x12);
  EXPECT_EQ(result[1], 0x34);
  EXPECT_EQ(result[2], 0x56);
  EXPECT_EQ(result[3], 0x78);
}

TEST_F(endian_tests, to_bytes_be_uint64)
{
  auto result = to_bytes_be<std::uint64_t>(0x123456789ABCDEF0ULL);

  ASSERT_EQ(result.size(), 8);
  EXPECT_EQ(result[0], 0x12);
  EXPECT_EQ(result[1], 0x34);
  EXPECT_EQ(result[2], 0x56);
  EXPECT_EQ(result[3], 0x78);
  EXPECT_EQ(result[4], 0x9A);
  EXPECT_EQ(result[5], 0xBC);
  EXPECT_EQ(result[6], 0xDE);
  EXPECT_EQ(result[7], 0xF0);
}

// ===== to_bytes_le Tests =====

TEST_F(endian_tests, to_bytes_le_uint8)
{
  auto result = to_bytes_le<std::uint8_t>(0xAB);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0xAB);
}

TEST_F(endian_tests, to_bytes_le_uint16)
{
  auto result = to_bytes_le<std::uint16_t>(0x1234);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x34);
  EXPECT_EQ(result[1], 0x12);
}

TEST_F(endian_tests, to_bytes_le_uint32)
{
  auto result = to_bytes_le<std::uint32_t>(0x12345678);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x78);
  EXPECT_EQ(result[1], 0x56);
  EXPECT_EQ(result[2], 0x34);
  EXPECT_EQ(result[3], 0x12);
}

TEST_F(endian_tests, to_bytes_le_uint64)
{
  auto result = to_bytes_le<std::uint64_t>(0x123456789ABCDEF0ULL);

  ASSERT_EQ(result.size(), 8);
  EXPECT_EQ(result[0], 0xF0);
  EXPECT_EQ(result[1], 0xDE);
  EXPECT_EQ(result[2], 0xBC);
  EXPECT_EQ(result[3], 0x9A);
  EXPECT_EQ(result[4], 0x78);
  EXPECT_EQ(result[5], 0x56);
  EXPECT_EQ(result[6], 0x34);
  EXPECT_EQ(result[7], 0x12);
}

// ===== Roundtrip Tests =====

TEST_F(endian_tests, read_write_be_roundtrip_uint16)
{
  std::array<std::uint8_t, 2> buffer{};
  std::uint16_t original = 0x1234;

  write_be<std::uint16_t>(buffer, original);
  auto restored = read_be<std::uint16_t>(buffer);

  EXPECT_EQ(restored, original);
}

TEST_F(endian_tests, read_write_be_roundtrip_uint32)
{
  std::array<std::uint8_t, 4> buffer{};
  std::uint32_t original = 0x12345678;

  write_be<std::uint32_t>(buffer, original);
  auto restored = read_be<std::uint32_t>(buffer);

  EXPECT_EQ(restored, original);
}

TEST_F(endian_tests, read_write_le_roundtrip_uint16)
{
  std::array<std::uint8_t, 2> buffer{};
  std::uint16_t original = 0x1234;

  write_le<std::uint16_t>(buffer, original);
  auto restored = read_le<std::uint16_t>(buffer);

  EXPECT_EQ(restored, original);
}

TEST_F(endian_tests, read_write_le_roundtrip_uint32)
{
  std::array<std::uint8_t, 4> buffer{};
  std::uint32_t original = 0x12345678;

  write_le<std::uint32_t>(buffer, original);
  auto restored = read_le<std::uint32_t>(buffer);

  EXPECT_EQ(restored, original);
}

TEST_F(endian_tests, to_bytes_read_be_roundtrip)
{
  std::uint32_t original = 0x12345678;

  auto bytes = to_bytes_be<std::uint32_t>(original);
  auto restored = read_be<std::uint32_t>(bytes);

  EXPECT_EQ(restored, original);
}

TEST_F(endian_tests, to_bytes_read_le_roundtrip)
{
  std::uint32_t original = 0x12345678;

  auto bytes = to_bytes_le<std::uint32_t>(original);
  auto restored = read_le<std::uint32_t>(bytes);

  EXPECT_EQ(restored, original);
}

// ===== Consistency Tests =====

TEST_F(endian_tests, big_endian_conversion_consistency)
{
  std::uint32_t value = 0x12345678;

  // These should produce the same result
  auto bytes1 = to_bytes_be<std::uint32_t>(value);

  std::array<std::uint8_t, 4> bytes2{};
  write_be<std::uint32_t>(bytes2, value);

  for (std::size_t i = 0; i < 4; ++i)
  {
    EXPECT_EQ(bytes1[i], bytes2[i]);
  }
}

TEST_F(endian_tests, little_endian_conversion_consistency)
{
  std::uint32_t value = 0x12345678;

  // These should produce the same result
  auto bytes1 = to_bytes_le<std::uint32_t>(value);

  std::array<std::uint8_t, 4> bytes2{};
  write_le<std::uint32_t>(bytes2, value);

  for (std::size_t i = 0; i < 4; ++i)
  {
    EXPECT_EQ(bytes1[i], bytes2[i]);
  }
}

// ===== Edge Cases =====

TEST_F(endian_tests, multiple_conversions)
{
  std::vector<std::uint16_t> values = {0x0000, 0x0001, 0x00FF, 0x0100, 0x1234, 0xABCD, 0xFFFF};

  for (auto value : values)
  {
    auto bytes_be = to_bytes_be<std::uint16_t>(value);
    auto restored_be = read_be<std::uint16_t>(bytes_be);
    EXPECT_EQ(restored_be, value);

    auto bytes_le = to_bytes_le<std::uint16_t>(value);
    auto restored_le = read_le<std::uint16_t>(bytes_le);
    EXPECT_EQ(restored_le, value);
  }
}

TEST_F(endian_tests, pattern_preservation)
{
  // Test that specific bit patterns are preserved correctly
  std::uint32_t patterns[] = {0x00000000, 0xFFFFFFFF, 0x01010101, 0x10101010, 0xAAAAAAAA, 0x55555555};

  for (auto pattern : patterns)
  {
    auto bytes = to_bytes_be<std::uint32_t>(pattern);
    auto restored = read_be<std::uint32_t>(bytes);
    EXPECT_EQ(restored, pattern);
  }
}

} // namespace carbio::unit_tests
