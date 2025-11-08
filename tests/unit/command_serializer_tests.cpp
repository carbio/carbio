#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fingerprint/command_serializer.h"
#include "fingerprint/command_traits.h"
#include "utility/endian.h"
#include "utility/scoped_zero.h"

#include <array>
#include <vector>

namespace carbio::unit_tests
{

class command_serializer_tests : public testing::Test
{
protected:
};

// ===== Request Serialization Tests =====

TEST_F(command_serializer_tests, serialize_capture_image)
{
  command_traits<command_code::capture_image>::request req{};
  auto result = serialize_request<command_code::capture_image>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_extract_features)
{
  command_traits<command_code::extract_features>::request req{0x01};
  auto result = serialize_request<command_code::extract_features>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x01);
}

TEST_F(command_serializer_tests, serialize_extract_features_buffer_id_2)
{
  command_traits<command_code::extract_features>::request req{0x02};
  auto result = serialize_request<command_code::extract_features>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x02);
}

TEST_F(command_serializer_tests, serialize_merge_model)
{
  command_traits<command_code::merge_model>::request req{};
  auto result = serialize_request<command_code::merge_model>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_store_model)
{
  command_traits<command_code::store_model>::request req{0x01, 0x1234};
  auto result = serialize_request<command_code::store_model>(req);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x01);
  // Verify big-endian encoding of page_id
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0x1234);
}

TEST_F(command_serializer_tests, serialize_store_model_different_values)
{
  command_traits<command_code::store_model>::request req{0x02, 0xABCD};
  auto result = serialize_request<command_code::store_model>(req);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x02);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0xABCD);
}

TEST_F(command_serializer_tests, serialize_load_model)
{
  command_traits<command_code::load_model>::request req{0x01, 0x5678};
  auto result = serialize_request<command_code::load_model>(req);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x01);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0x5678);
}

TEST_F(command_serializer_tests, serialize_upload_model)
{
  command_traits<command_code::upload_model>::request req{0x01};
  auto result = serialize_request<command_code::upload_model>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x01);
}

TEST_F(command_serializer_tests, serialize_download_model)
{
  command_traits<command_code::download_model>::request req{0x02};
  auto result = serialize_request<command_code::download_model>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x02);
}

TEST_F(command_serializer_tests, serialize_upload_image)
{
  command_traits<command_code::upload_image>::request req{};
  auto result = serialize_request<command_code::upload_image>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_download_image)
{
  command_traits<command_code::download_image>::request req{};
  auto result = serialize_request<command_code::download_image>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_erase_model)
{
  command_traits<command_code::erase_model>::request req{0x0010, 0x0020};
  auto result = serialize_request<command_code::erase_model>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data(), 2));
  EXPECT_EQ(page_id, 0x0010);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 2, 2));
  EXPECT_EQ(count, 0x0020);
}

TEST_F(command_serializer_tests, serialize_erase_model_max_values)
{
  command_traits<command_code::erase_model>::request req{0xFFFF, 0xFFFF};
  auto result = serialize_request<command_code::erase_model>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data(), 2));
  EXPECT_EQ(page_id, 0xFFFF);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 2, 2));
  EXPECT_EQ(count, 0xFFFF);
}

TEST_F(command_serializer_tests, serialize_clear_database)
{
  command_traits<command_code::clear_database>::request req{};
  auto result = serialize_request<command_code::clear_database>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_search_model)
{
  command_traits<command_code::search_model>::request req{0x01, 0x0000, 0x00FF};
  auto result = serialize_request<command_code::search_model>(req);

  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 0x01);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0x0000);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 3, 2));
  EXPECT_EQ(count, 0x00FF);
}

TEST_F(command_serializer_tests, serialize_fast_search_model)
{
  command_traits<command_code::fast_search_model>::request req{0x02, 0x0100, 0x0200};
  auto result = serialize_request<command_code::fast_search_model>(req);

  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 0x02);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0x0100);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 3, 2));
  EXPECT_EQ(count, 0x0200);
}

TEST_F(command_serializer_tests, serialize_match_model)
{
  command_traits<command_code::match_model>::request req{};
  auto result = serialize_request<command_code::match_model>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_set_device_password)
{
  scoped_zero<std::uint32_t> password(0x12345678);
  command_traits<command_code::set_device_password>::request req{password};
  auto result = serialize_request<command_code::set_device_password>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint32_t encoded_password = read_be<std::uint32_t>(std::span(result.data(), 4));
  EXPECT_EQ(encoded_password, 0x12345678);
}

TEST_F(command_serializer_tests, serialize_verify_device_password)
{
  scoped_zero<std::uint32_t> password(0xABCDEF00);
  command_traits<command_code::verify_device_password>::request req{password};
  auto result = serialize_request<command_code::verify_device_password>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint32_t encoded_password = read_be<std::uint32_t>(std::span(result.data(), 4));
  EXPECT_EQ(encoded_password, 0xABCDEF00);
}

TEST_F(command_serializer_tests, serialize_write_system_parameter)
{
  command_traits<command_code::write_system_parameter>::request req{0x04, 0x05};
  auto result = serialize_request<command_code::write_system_parameter>(req);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x04);
  EXPECT_EQ(result[1], 0x05);
}

TEST_F(command_serializer_tests, serialize_read_system_parameter)
{
  command_traits<command_code::read_system_parameter>::request req{};
  auto result = serialize_request<command_code::read_system_parameter>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_set_led_config)
{
  command_traits<command_code::set_led_config>::request req{0x01, 0x02, 0x03, 0x04};
  auto result = serialize_request<command_code::set_led_config>(req);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x01);
  EXPECT_EQ(result[1], 0x02);
  EXPECT_EQ(result[2], 0x03);
  EXPECT_EQ(result[3], 0x04);
}

TEST_F(command_serializer_tests, serialize_write_notepad)
{
  std::array<std::uint8_t, 32> notepad_data;
  std::ranges::fill(notepad_data, 0xAA);
  command_traits<command_code::write_notepad>::request req{0x0F, notepad_data};
  auto result = serialize_request<command_code::write_notepad>(req);

  ASSERT_EQ(result.size(), 33);
  EXPECT_EQ(result[0], 0x0F);
  for (std::size_t i = 1; i < 33; ++i)
  {
    EXPECT_EQ(result[i], 0xAA);
  }
}

TEST_F(command_serializer_tests, serialize_read_notepad)
{
  command_traits<command_code::read_notepad>::request req{0x0A};
  auto result = serialize_request<command_code::read_notepad>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x0A);
}

TEST_F(command_serializer_tests, serialize_read_index_table)
{
  command_traits<command_code::read_index_table>::request req{0x03};
  auto result = serialize_request<command_code::read_index_table>(req);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x03);
}

TEST_F(command_serializer_tests, serialize_count_model)
{
  command_traits<command_code::count_model>::request req{};
  auto result = serialize_request<command_code::count_model>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_turn_led_on)
{
  command_traits<command_code::turn_led_on>::request req{};
  auto result = serialize_request<command_code::turn_led_on>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_turn_led_off)
{
  command_traits<command_code::turn_led_off>::request req{};
  auto result = serialize_request<command_code::turn_led_off>(req);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(command_serializer_tests, serialize_soft_reset_device)
{
  command_traits<command_code::soft_reset_device>::request req{};
  auto result = serialize_request<command_code::soft_reset_device>(req);

  EXPECT_EQ(result.size(), 0);
}

// ===== Edge Cases and Boundary Tests =====

TEST_F(command_serializer_tests, serialize_store_model_zero_values)
{
  command_traits<command_code::store_model>::request req{0x00, 0x0000};
  auto result = serialize_request<command_code::store_model>(req);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x00);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0x0000);
}

TEST_F(command_serializer_tests, serialize_store_model_max_page_id)
{
  command_traits<command_code::store_model>::request req{0xFF, 0xFFFF};
  auto result = serialize_request<command_code::store_model>(req);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0xFF);
  std::uint16_t page_id = read_be<std::uint16_t>(std::span(result.data() + 1, 2));
  EXPECT_EQ(page_id, 0xFFFF);
}

TEST_F(command_serializer_tests, serialize_search_model_zero_count)
{
  command_traits<command_code::search_model>::request req{0x01, 0x0000, 0x0000};
  auto result = serialize_request<command_code::search_model>(req);

  ASSERT_EQ(result.size(), 5);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 3, 2));
  EXPECT_EQ(count, 0x0000);
}

TEST_F(command_serializer_tests, serialize_search_model_max_count)
{
  command_traits<command_code::search_model>::request req{0x01, 0x0000, 0xFFFF};
  auto result = serialize_request<command_code::search_model>(req);

  ASSERT_EQ(result.size(), 5);
  std::uint16_t count = read_be<std::uint16_t>(std::span(result.data() + 3, 2));
  EXPECT_EQ(count, 0xFFFF);
}

TEST_F(command_serializer_tests, serialize_set_device_password_zero)
{
  scoped_zero<std::uint32_t> password(0x00000000);
  command_traits<command_code::set_device_password>::request req{password};
  auto result = serialize_request<command_code::set_device_password>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint32_t encoded_password = read_be<std::uint32_t>(std::span(result.data(), 4));
  EXPECT_EQ(encoded_password, 0x00000000);
}

TEST_F(command_serializer_tests, serialize_set_device_password_max)
{
  scoped_zero<std::uint32_t> password(0xFFFFFFFF);
  command_traits<command_code::set_device_password>::request req{password};
  auto result = serialize_request<command_code::set_device_password>(req);

  ASSERT_EQ(result.size(), 4);
  std::uint32_t encoded_password = read_be<std::uint32_t>(std::span(result.data(), 4));
  EXPECT_EQ(encoded_password, 0xFFFFFFFF);
}

TEST_F(command_serializer_tests, serialize_write_notepad_empty_data)
{
  std::array<std::uint8_t, 32> notepad_data{};
  command_traits<command_code::write_notepad>::request req{0x00, notepad_data};
  auto result = serialize_request<command_code::write_notepad>(req);

  ASSERT_EQ(result.size(), 33);
  EXPECT_EQ(result[0], 0x00);
  for (std::size_t i = 1; i < 33; ++i)
  {
    EXPECT_EQ(result[i], 0x00);
  }
}

TEST_F(command_serializer_tests, serialize_write_notepad_pattern_data)
{
  std::array<std::uint8_t, 32> notepad_data;
  for (std::size_t i = 0; i < 32; ++i)
  {
    notepad_data[i] = static_cast<std::uint8_t>(i);
  }
  command_traits<command_code::write_notepad>::request req{0x05, notepad_data};
  auto result = serialize_request<command_code::write_notepad>(req);

  ASSERT_EQ(result.size(), 33);
  EXPECT_EQ(result[0], 0x05);
  for (std::size_t i = 0; i < 32; ++i)
  {
    EXPECT_EQ(result[i + 1], static_cast<std::uint8_t>(i));
  }
}

TEST_F(command_serializer_tests, serialize_set_led_config_all_zeros)
{
  command_traits<command_code::set_led_config>::request req{0x00, 0x00, 0x00, 0x00};
  auto result = serialize_request<command_code::set_led_config>(req);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x00);
  EXPECT_EQ(result[1], 0x00);
  EXPECT_EQ(result[2], 0x00);
  EXPECT_EQ(result[3], 0x00);
}

TEST_F(command_serializer_tests, serialize_set_led_config_all_max)
{
  command_traits<command_code::set_led_config>::request req{0xFF, 0xFF, 0xFF, 0xFF};
  auto result = serialize_request<command_code::set_led_config>(req);

  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0xFF);
  EXPECT_EQ(result[1], 0xFF);
  EXPECT_EQ(result[2], 0xFF);
  EXPECT_EQ(result[3], 0xFF);
}

// ===== Consistency Tests =====

TEST_F(command_serializer_tests, serialize_multiple_calls_same_data)
{
  command_traits<command_code::store_model>::request req{0x01, 0x1234};

  auto result1 = serialize_request<command_code::store_model>(req);
  auto result2 = serialize_request<command_code::store_model>(req);

  ASSERT_EQ(result1.size(), result2.size());
  for (std::size_t i = 0; i < result1.size(); ++i)
  {
    EXPECT_EQ(result1[i], result2[i]);
  }
}

TEST_F(command_serializer_tests, serialize_different_buffer_ids)
{
  std::vector<std::uint8_t> buffer_ids = {0x00, 0x01, 0x02, 0xFF};

  for (auto buffer_id : buffer_ids)
  {
    command_traits<command_code::upload_model>::request req{buffer_id};
    auto result = serialize_request<command_code::upload_model>(req);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], buffer_id);
  }
}

} // namespace carbio::unit_tests
