#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fingerprint/command_code.h"
#include "fingerprint/command_executor.h"
#include "fingerprint/packet.h"
#include "fingerprint/status_code.h"
#include "protocol_handler_mock.h"
#include "serial_port_mock.h"
#include "utility/endian.h"

#include <array>
#include <chrono>
#include <vector>

namespace carbio::unit_tests
{
using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::SetArrayArgument;
using testing::StrictMock;

class command_executor_tests : public testing::Test
{
protected:
  StrictMock<serial_port_mock> serial_mock;
  StrictMock<protocol_handler_mock> protocol_mock;

  // Helper to create a valid command packet
  locked_buffer<std::uint8_t> create_command_packet(std::uint8_t cmd_code)
  {
    locked_buffer<std::uint8_t> buffer(12); // Correct size: 9 header + 1 cmd + 2 checksum
    auto span = buffer.as_span();

    // Tag
    span[0] = 0xEF;
    span[1] = 0x01;

    // Address
    span[2] = 0xFF;
    span[3] = 0xFF;
    span[4] = 0xFF;
    span[5] = 0xFF;

    // Type (command)
    span[6] = 0x01;

    // Length (command code + checksum = 3)
    span[7] = 0x00;
    span[8] = 0x03;

    // Command code
    span[9] = cmd_code;

    // Checksum (simplified)
    std::uint16_t checksum = 0x01 + 0x03 + cmd_code;
    auto checksum_bytes = to_bytes_be(checksum);
    span[10] = checksum_bytes[0];
    span[11] = checksum_bytes[1];

    return buffer;
  }

  // Helper to create an acknowledgment packet
  locked_buffer<std::uint8_t> create_ack_packet(status_code status)
  {
    locked_buffer<std::uint8_t> buffer(12);
    auto span = buffer.as_span();

    // Tag
    span[0] = 0xEF;
    span[1] = 0x01;

    // Address
    span[2] = 0xFF;
    span[3] = 0xFF;
    span[4] = 0xFF;
    span[5] = 0xFF;

    // Type (acknowledge)
    span[6] = 0x07;

    // Length (status + checksum = 3)
    span[7] = 0x00;
    span[8] = 0x03;

    // Status
    span[9] = static_cast<std::uint8_t>(status);

    // Checksum
    std::uint16_t checksum = 0x07 + 0x03 + static_cast<std::uint8_t>(status);
    auto checksum_bytes = to_bytes_be(checksum);
    span[10] = checksum_bytes[0];
    span[11] = checksum_bytes[1];

    return buffer;
  }

  // Helper to create acknowledgment response data
  locked_buffer<std::uint8_t> create_ack_response(status_code, std::span<const std::uint8_t> data = {})
  {
    locked_buffer<std::uint8_t> buffer(data.size());
    if (!data.empty())
    {
      buffer.copy_from(data.data(), data.size());
    }
    return buffer;
  }
};

// ===== Execute Command Tests - Success Cases =====

TEST_F(command_executor_tests, execute_capture_image_success)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  // Setup expectations
  EXPECT_CALL(serial_mock, flush()).Times(1);

  // Construct command packet
  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, _))
      .WillOnce(
          [this](command_code, std::span<const std::uint8_t>)
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          });

  // Write packet
  EXPECT_CALL(serial_mock, write_exact(_, _))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            return data.size();
          }));

  // Drain
  EXPECT_CALL(serial_mock, drain()).Times(1);

  // Read header - expect size 9
  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 9;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            // Simulate reading header
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07; // ack type
            data[7] = 0x00; // length high
            data[8] = 0x03; // length low (status + checksum)
            return data.size();
          }));

  // Read body - expect size 3
  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 3;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            // Status byte + checksum
            data[0] = 0x00; // success
            data[1] = 0x00; // checksum high
            data[2] = 0x0A; // checksum low
            return data.size();
          }));

  // Parse acknowledgment
  EXPECT_CALL(protocol_mock, parse_acknowledge_packet(_))
      .WillOnce(
          [this](std::span<const std::uint8_t>)
          {
            auto ack_response = create_ack_response(status_code::success);
            return make_success(std::move(ack_response));
          });

  // Execute
  auto result = executor.execute<command_code::capture_image>(req);

  if (!result.has_value())
  {
    std::cerr << "Error code: " << static_cast<int>(result.error()) << std::endl;
  }
  ASSERT_TRUE(result.has_value());
}

TEST_F(command_executor_tests, execute_merge_model_success)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::merge_model>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::merge_model, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x05);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 9;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07;
            data[7] = 0x00;
            data[8] = 0x03;
            return data.size();
          }));

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 3;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0x00;
            data[1] = 0x00;
            data[2] = 0x0A;
            return data.size();
          }));

  EXPECT_CALL(protocol_mock, parse_acknowledge_packet(_))
      .WillOnce(
          [this](std::span<const std::uint8_t>)
          {
            auto ack_response = create_ack_response(status_code::success);
            return make_success(std::move(ack_response));
          });

  auto result = executor.execute<command_code::merge_model>(req);

  ASSERT_TRUE(result.has_value());
}

TEST_F(command_executor_tests, execute_match_model_success)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::match_model>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::match_model, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x03);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 9;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07;
            data[7] = 0x00;
            data[8] = 0x05; // length includes score data
            return data.size();
          }));

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 5;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0x00; // success
            data[1] = 0x01; // score high
            data[2] = 0x23; // score low
            data[3] = 0x00; // checksum high
            data[4] = 0x29; // checksum low
            return data.size();
          }));

  // Create response with score data
  std::array<std::uint8_t, 2> score_data{0x01, 0x23};
  EXPECT_CALL(protocol_mock, parse_acknowledge_packet(_))
      .WillOnce(
          [this, &score_data]()
          {
            auto ack_response = create_ack_response(status_code::success, score_data);
            return make_success(std::move(ack_response));
          });

  auto result = executor.execute<command_code::match_model>(req);

  ASSERT_TRUE(result.has_value());
}

// ===== Execute Command Tests - Error Cases =====

TEST_F(command_executor_tests, execute_construct_packet_fails)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, _)).WillOnce(Return(make_error(status_code::bad_packet)));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(command_executor_tests, execute_write_timeout)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  std::size_t expected_size = 12; // Size from create_command_packet
  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          }));

  // Write returns less than expected
  EXPECT_CALL(serial_mock, write_exact(_, _)).WillOnce(Return(expected_size - 1));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::timeout);
}

TEST_F(command_executor_tests, execute_read_header_timeout)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  // Read header returns less than expected
  EXPECT_CALL(serial_mock, read_exact(_, _)).WillOnce(Return(5));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(command_executor_tests, execute_invalid_length_in_header)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  EXPECT_CALL(serial_mock, read_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07;
            data[7] = 0x00;
            data[8] = 0x01; // length < 2 (invalid)
            return data.size();
          }));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(command_executor_tests, execute_read_body_timeout)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 9;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07;
            data[7] = 0x00;
            data[8] = 0x03;
            return data.size();
          }));

  // Read body returns less than expected
  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 3;
                                          }),
                                      _))
      .WillOnce(Return(1));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::frame_error);
}

TEST_F(command_executor_tests, execute_parse_ack_fails)
{
  command_executor executor(serial_mock, protocol_mock);
  command_traits<command_code::capture_image>::request req{};

  EXPECT_CALL(serial_mock, flush()).Times(1);

  EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
      .WillOnce(testing::Invoke(
          [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
          {
            auto cmd_packet = create_command_packet(0x01);
            return make_success(std::move(cmd_packet));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 9;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0xEF;
            data[1] = 0x01;
            data[2] = 0xFF;
            data[3] = 0xFF;
            data[4] = 0xFF;
            data[5] = 0xFF;
            data[6] = 0x07;
            data[7] = 0x00;
            data[8] = 0x03;
            return data.size();
          }));

  EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                          [](std::span<std::uint8_t> s)
                                          {
                                            return s.size() == 3;
                                          }),
                                      testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<std::uint8_t> data, std::chrono::milliseconds) -> std::size_t
          {
            data[0] = 0x02; // no_finger status
            data[1] = 0x00;
            data[2] = 0x0C;
            return data.size();
          }));

  EXPECT_CALL(protocol_mock, parse_acknowledge_packet(_)).WillOnce(Return(make_error(status_code::no_finger)));

  auto result = executor.execute<command_code::capture_image>(req);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::no_finger);
}

// ===== Send Data Packets Tests =====

TEST_F(command_executor_tests, send_data_packets_success)
{
  command_executor executor(serial_mock, protocol_mock);

  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  EXPECT_CALL(protocol_mock, construct_data_packet(testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t>) noexcept -> result<std::vector<locked_buffer<std::uint8_t>>>
          {
            std::vector<locked_buffer<std::uint8_t>> packets;
            locked_buffer<std::uint8_t> packet1(15);
            packets.push_back(std::move(packet1));
            return make_success(std::move(packets));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t> v, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return v.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  auto result = executor.send_data_packets(data);

  ASSERT_TRUE(result.has_value());
}

TEST_F(command_executor_tests, send_data_packets_construct_fails)
{
  command_executor executor(serial_mock, protocol_mock);

  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  EXPECT_CALL(protocol_mock, construct_data_packet(_)).WillOnce(Return(make_error(status_code::bad_packet)));

  auto result = executor.send_data_packets(data);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::bad_packet);
}

TEST_F(command_executor_tests, send_data_packets_write_timeout)
{
  command_executor executor(serial_mock, protocol_mock);

  std::array<std::uint8_t, 10> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  EXPECT_CALL(protocol_mock, construct_data_packet(testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t>) noexcept -> result<std::vector<locked_buffer<std::uint8_t>>>
          {
            std::vector<locked_buffer<std::uint8_t>> packets;
            locked_buffer<std::uint8_t> packet1(15);
            packets.push_back(std::move(packet1));
            return make_success(std::move(packets));
          }));

  // Write returns less than expected
  EXPECT_CALL(serial_mock, write_exact(_, _)).WillOnce(Return(10));

  auto result = executor.send_data_packets(data);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), status_code::timeout);
}

TEST_F(command_executor_tests, send_data_packets_multiple_packets)
{
  command_executor executor(serial_mock, protocol_mock);

  std::array<std::uint8_t, 300> data{};

  EXPECT_CALL(protocol_mock, construct_data_packet(testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t>) noexcept -> result<std::vector<locked_buffer<std::uint8_t>>>
          {
            std::vector<locked_buffer<std::uint8_t>> packets;
            locked_buffer<std::uint8_t> packet1(150);
            locked_buffer<std::uint8_t> packet2(150);
            locked_buffer<std::uint8_t> packet3(20);
            packets.push_back(std::move(packet1));
            packets.push_back(std::move(packet2));
            packets.push_back(std::move(packet3));
            return make_success(std::move(packets));
          }));

  EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
      .Times(3)
      .WillRepeatedly(testing::Invoke(
          [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
          {
            return data.size();
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  auto result = executor.send_data_packets(data);

  ASSERT_TRUE(result.has_value());
}

TEST_F(command_executor_tests, send_data_packets_empty)
{
  command_executor executor(serial_mock, protocol_mock);

  std::vector<std::uint8_t> data;

  EXPECT_CALL(protocol_mock, construct_data_packet(testing::_))
      .WillOnce(testing::Invoke(
          [](std::span<const std::uint8_t>) noexcept -> result<std::vector<locked_buffer<std::uint8_t>>>
          {
            std::vector<locked_buffer<std::uint8_t>> packets; // Empty
            return make_success(std::move(packets));
          }));

  EXPECT_CALL(serial_mock, drain()).Times(1);

  auto result = executor.send_data_packets(data);

  ASSERT_TRUE(result.has_value());
}

// ===== Integration-like Tests =====

TEST_F(command_executor_tests, execute_multiple_commands_sequentially)
{
  command_executor executor(serial_mock, protocol_mock);

  // First command: capture_image
  {
    command_traits<command_code::capture_image>::request req{};

    EXPECT_CALL(serial_mock, flush()).Times(1);

    EXPECT_CALL(protocol_mock, construct_command_packet(command_code::capture_image, testing::_))
        .WillOnce(testing::Invoke(
            [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
            {
              auto cmd_packet = create_command_packet(0x01);
              return make_success(std::move(cmd_packet));
            }));

    EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              return data.size();
            }));

    EXPECT_CALL(serial_mock, drain()).Times(1);

    // Read header - expect size 9
    EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                            [](std::span<std::uint8_t> s)
                                            {
                                              return s.size() == 9;
                                            }),
                                        testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              data[0] = 0xEF;
              data[1] = 0x01;
              data[2] = 0xFF;
              data[3] = 0xFF;
              data[4] = 0xFF;
              data[5] = 0xFF;
              data[6] = 0x07;
              data[7] = 0x00;
              data[8] = 0x03;
              return data.size();
            }));

    // Read body - expect size 3
    EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                            [](std::span<std::uint8_t> s)
                                            {
                                              return s.size() == 3;
                                            }),
                                        testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              data[0] = 0x00;
              data[1] = 0x00;
              data[2] = 0x0A;
              return data.size();
            }));

    EXPECT_CALL(protocol_mock, parse_acknowledge_packet(testing::_))
        .WillOnce(testing::Invoke(
            [this](std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
            {
              auto ack_response = create_ack_response(status_code::success);
              return make_success(std::move(ack_response));
            }));

    auto result = executor.execute<command_code::capture_image>(req);

    ASSERT_TRUE(result.has_value());
  }

  // Second command: merge_model
  {
    command_traits<command_code::merge_model>::request req{};

    EXPECT_CALL(serial_mock, flush()).Times(1);

    EXPECT_CALL(protocol_mock, construct_command_packet(command_code::merge_model, testing::_))
        .WillOnce(testing::Invoke(
            [this](command_code, std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
            {
              auto cmd_packet = create_command_packet(0x05);
              return make_success(std::move(cmd_packet));
            }));

    EXPECT_CALL(serial_mock, write_exact(testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<const std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              return data.size();
            }));

    EXPECT_CALL(serial_mock, drain()).Times(1);

    // Read header - expect size 9
    EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                            [](std::span<std::uint8_t> s)
                                            {
                                              return s.size() == 9;
                                            }),
                                        testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              data[0] = 0xEF;
              data[1] = 0x01;
              data[2] = 0xFF;
              data[3] = 0xFF;
              data[4] = 0xFF;
              data[5] = 0xFF;
              data[6] = 0x07;
              data[7] = 0x00;
              data[8] = 0x03;
              return data.size();
            }));

    // Read body - expect size 3
    EXPECT_CALL(serial_mock, read_exact(testing::Truly(
                                            [](std::span<std::uint8_t> s)
                                            {
                                              return s.size() == 3;
                                            }),
                                        testing::_))
        .WillOnce(testing::Invoke(
            [](std::span<std::uint8_t> data, std::chrono::milliseconds) noexcept -> std::size_t
            {
              data[0] = 0x00;
              data[1] = 0x00;
              data[2] = 0x0A;
              return data.size();
            }));

    EXPECT_CALL(protocol_mock, parse_acknowledge_packet(testing::_))
        .WillOnce(testing::Invoke(
            [this](std::span<const std::uint8_t>) noexcept -> result<locked_buffer<std::uint8_t>>
            {
              auto ack_response = create_ack_response(status_code::success);
              return make_success(std::move(ack_response));
            }));

    auto result = executor.execute<command_code::merge_model>(req);

    ASSERT_TRUE(result.has_value());
  }
}

} // namespace carbio::unit_tests
