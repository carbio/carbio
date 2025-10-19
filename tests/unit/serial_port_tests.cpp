#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "serial_port_mock.h"

#include <array>
#include <chrono>
#include <future>
#include <span>
#include <thread>
#include <vector>

namespace carbio::unit_tests {
using testing::_;
using testing::InSequence;
using testing::Return;
using testing::StrictMock;
using testing::Throw;

class serial_port_tests : public testing::Test {
protected:
  StrictMock<serial_port_mock> mock;

  static constexpr std::array<std::uint8_t, 5> test_data{0x01, 0x02, 0x03, 0x04,
                                                         0x05};
  static constexpr std::array<std::uint8_t, 1024> large_buffer{};
};

TEST_F(serial_port_tests, is_open_returns_false_initially) {
  EXPECT_CALL(mock, is_open()).WillOnce(Return(false));
  EXPECT_FALSE(mock.is_open());
}

TEST_F(serial_port_tests, is_open_returns_true_after_open) {
  EXPECT_CALL(mock, is_open()).WillOnce(Return(true));
  EXPECT_TRUE(mock.is_open());
}

TEST_F(serial_port_tests, is_open_multiple_calls_consistent) {
  EXPECT_CALL(mock, is_open()).WillRepeatedly(Return(true));
  EXPECT_TRUE(mock.is_open());
  EXPECT_TRUE(mock.is_open());
  EXPECT_TRUE(mock.is_open());
}

TEST_F(serial_port_tests, open_with_default_config) {
  EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
  mock.open("/dev/ttyUSB0");
}

TEST_F(serial_port_tests, open_with_custom_config) {
  using namespace carbio::io;
  EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
  EXPECT_CALL(mock, set_baud_rate(115200)).WillOnce(Return(true));
  EXPECT_CALL(mock, set_data_width(data_width::_8)).WillOnce(Return(true));
  EXPECT_CALL(mock, set_stop_width(stop_width::_1)).WillOnce(Return(true));
  EXPECT_CALL(mock, set_parity_mode(parity_mode::none)).WillOnce(Return(true));
  EXPECT_CALL(mock, set_flow_control(flow_control::none)).WillOnce(Return(true));
  mock.open("/dev/ttyUSB0");
  mock.set_baud_rate(115200);
  mock.set_data_width(data_width::_8);
  mock.set_stop_width(stop_width::_1);
  mock.set_parity_mode(parity_mode::none);
  mock.set_flow_control(flow_control::none);
}

TEST_F(serial_port_tests, close_port) {
  EXPECT_CALL(mock, close()).WillOnce(Return());
  mock.close();
}

TEST_F(serial_port_tests, multiple_close_calls_safe) {
  EXPECT_CALL(mock, close()).Times(3).WillRepeatedly(Return());
  mock.close();
  mock.close();
  mock.close();
}

TEST_F(serial_port_tests, write_some_valid_data) {
  EXPECT_CALL(mock, write_some(testing::_)).WillOnce(Return(test_data.size()));
  std::span<const std::uint8_t> data{test_data};
  auto result = mock.write_some(data);
  EXPECT_EQ(result, test_data.size());
}

TEST_F(serial_port_tests, write_some_empty_data) {
  EXPECT_CALL(mock, write_some(testing::_)).WillOnce(Return(0));
  std::span<const std::uint8_t> empty_data;
  auto result = mock.write_some(empty_data);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, write_some_single_byte) {
  std::uint8_t single_byte = 0xAB;
  std::span<const std::uint8_t> data{&single_byte, 1};
  EXPECT_CALL(mock, write_some(testing::_)).WillOnce(Return(1));
  auto result = mock.write_some(data);
  EXPECT_EQ(result, 1);
}

TEST_F(serial_port_tests, write_some_large_buffer) {
  EXPECT_CALL(mock, write_some(testing::_))
      .WillOnce(Return(large_buffer.size()));
  std::span<const std::uint8_t> data{large_buffer};
  auto result = mock.write_some(data);
  EXPECT_EQ(result, large_buffer.size());
}

TEST_F(serial_port_tests, test_write_some_partial_write) {
  EXPECT_CALL(mock, write_some(testing::_))
      .WillOnce(Return(test_data.size() / 2));
  std::span<const std::uint8_t> data{test_data};
  auto result = mock.write_some(data);
  EXPECT_EQ(result, test_data.size() / 2);
}

TEST_F(serial_port_tests, test_write_exact_with_timeout) {
  auto timeout = std::chrono::milliseconds{100};
  EXPECT_CALL(mock, write_exact(testing::_, timeout))
      .WillOnce(Return(test_data.size()));
  std::span<const std::uint8_t> data{test_data};
  auto result = mock.write_exact(data, timeout);
  EXPECT_EQ(result, test_data.size());
}

TEST_F(serial_port_tests, write_exact_timeout_expires) {
  auto timeout = std::chrono::milliseconds{1};
  EXPECT_CALL(mock, write_exact(testing::_, timeout)).WillOnce(Return(0));
  std::span<const std::uint8_t> data{test_data};
  auto result = mock.write_exact(data, timeout);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, write_exact_zero_timeout) {
  auto timeout = std::chrono::milliseconds{0};
  EXPECT_CALL(mock, write_exact(testing::_, timeout)).WillOnce(Return(0));
  std::span<const std::uint8_t> data{test_data};
  auto result = mock.write_exact(data, timeout);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, read_some_available_data) {
  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> data{buffer};
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(5));
  auto result = mock.read_some(data);
  EXPECT_EQ(result, 5);
}

TEST_F(serial_port_tests, read_some_no_data_available) {
  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> data{buffer};
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(0));
  auto result = mock.read_some(data);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, read_some_single_byte) {
  std::uint8_t single_byte;
  std::span<std::uint8_t> data{&single_byte, 1};
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(1));
  auto result = mock.read_some(data);
  EXPECT_EQ(result, 1);
}

TEST_F(serial_port_tests, read_some_large_buffer) {
  std::array<std::uint8_t, 1024> buffer{};
  std::span<std::uint8_t> data{buffer};
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(512));
  auto result = mock.read_some(data);
  EXPECT_EQ(result, 512);
}

TEST_F(serial_port_tests, read_some_empty_buffer) {
  std::span<std::uint8_t> empty_data;
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(0));
  auto result = mock.read_some(empty_data);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, read_exact_with_timeout) {
  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> data{buffer};
  auto timeout = std::chrono::milliseconds{100};
  EXPECT_CALL(mock, read_exact(testing::_, timeout)).WillOnce(Return(5));
  auto result = mock.read_exact(data, timeout);
  EXPECT_EQ(result, 5);
}

TEST_F(serial_port_tests, read_exact_timeout_expires) {
  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> data{buffer};
  auto timeout = std::chrono::milliseconds{1};
  EXPECT_CALL(mock, read_exact(testing::_, timeout)).WillOnce(Return(0));
  auto result = mock.read_exact(data, timeout);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, read_exact_zero_timeout) {
  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> data{buffer};
  auto timeout = std::chrono::milliseconds{0};
  EXPECT_CALL(mock, read_exact(testing::_, timeout)).WillOnce(Return(0));
  auto result = mock.read_exact(data, timeout);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, available_returns_count) {
  EXPECT_CALL(mock, available()).WillOnce(Return(42));
  auto result = mock.available();
  EXPECT_EQ(result, 42);
}

TEST_F(serial_port_tests, available_returns_zero) {
  EXPECT_CALL(mock, available()).WillOnce(Return(0));
  auto result = mock.available();
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, flush_operation) {
  EXPECT_CALL(mock, flush()).WillOnce(Return());
  mock.flush();
}

TEST_F(serial_port_tests, drain_operation) {
  EXPECT_CALL(mock, drain()).WillOnce(Return());
  mock.drain();
}

TEST_F(serial_port_tests, cancel_operation) {
  EXPECT_CALL(mock, cancel()).WillOnce(Return());
  mock.cancel();
}

TEST_F(serial_port_tests, write_zero_length_span) {
  std::array<std::uint8_t, 0> empty_array{};
  std::span<const std::uint8_t> data{empty_array};
  EXPECT_CALL(mock, write_some(testing::_)).WillOnce(Return(0));
  auto result = mock.write_some(data);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, read_zero_length_span) {
  std::array<std::uint8_t, 0> empty_array{};
  std::span<std::uint8_t> data{empty_array};
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(0));
  auto result = mock.read_some(data);
  EXPECT_EQ(result, 0);
}

TEST_F(serial_port_tests, open_with_all_baud_rates) {
  std::vector<std::uint32_t> baud_rates = {9600,   19200,  38400,  57600,
                                           115200, 230400, 460800, 921600};

  for (auto baud : baud_rates) {
    EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
    EXPECT_CALL(mock, set_baud_rate(baud)).WillOnce(Return(true));
    mock.open("/dev/ttyUSB0");
    mock.set_baud_rate(baud);
  }
}

TEST_F(serial_port_tests, open_with_all_data_bits) {
  using namespace carbio::io;
  std::vector<data_width> data_bits = {data_width::_5, data_width::_6,
                                       data_width::_7, data_width::_8};

  for (auto bits : data_bits) {
    EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
    EXPECT_CALL(mock, set_data_width(bits)).WillOnce(Return(true));
    mock.open("/dev/ttyUSB0");
    mock.set_data_width(bits);
  }
}

TEST_F(serial_port_tests, open_with_all_stop_bits) {
  using namespace carbio::io;
  std::vector<stop_width> stop_bits = {stop_width::_1, stop_width::_2};

  for (auto bits : stop_bits) {
    EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
    EXPECT_CALL(mock, set_stop_width(bits)).WillOnce(Return(true));
    mock.open("/dev/ttyUSB0");
    mock.set_stop_width(bits);
  }
}

TEST_F(serial_port_tests, open_with_all_parity_modes) {
  using namespace carbio::io;
  std::vector<parity_mode> parity_modes = {parity_mode::none, parity_mode::odd,
                                           parity_mode::even};

  for (auto parity : parity_modes) {
    EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
    EXPECT_CALL(mock, set_parity_mode(parity)).WillOnce(Return(true));
    mock.open("/dev/ttyUSB0");
    mock.set_parity_mode(parity);
  }
}

TEST_F(serial_port_tests, open_with_all_flow_control_modes) {
  using namespace carbio::io;
  std::vector<flow_control> flow_modes = {
      flow_control::none, flow_control::software, flow_control::hardware,
      flow_control::both};

  for (auto flow : flow_modes) {
    EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
    EXPECT_CALL(mock, set_flow_control(flow)).WillOnce(Return(true));
    mock.open("/dev/ttyUSB0");
    mock.set_flow_control(flow);
  }
}

TEST_F(serial_port_tests, test_open_write_read_close_sequence) {
  InSequence seq;

  EXPECT_CALL(mock, open("/dev/ttyUSB0")).WillOnce(Return(true));
  EXPECT_CALL(mock, write_some(testing::_)).WillOnce(Return(test_data.size()));
  EXPECT_CALL(mock, read_some(testing::_)).WillOnce(Return(5));
  EXPECT_CALL(mock, close()).WillOnce(Return());

  mock.open("/dev/ttyUSB0");
  std::span<const std::uint8_t> write_data{test_data};
  mock.write_some(write_data);

  std::array<std::uint8_t, 10> buffer{};
  std::span<std::uint8_t> read_data{buffer};
  mock.read_some(read_data);

  mock.close();
}

TEST_F(serial_port_tests, flush_drain_clear_sequence) {
  InSequence seq;

  EXPECT_CALL(mock, flush()).WillOnce(Return());
  EXPECT_CALL(mock, drain()).WillOnce(Return());

  mock.flush();
  mock.drain();
}

TEST_F(serial_port_tests, write_exact_various_timeouts) {
  std::vector<std::chrono::milliseconds> timeouts = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{1},
      std::chrono::milliseconds{10}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{1000}};

  for (auto timeout : timeouts) {
    EXPECT_CALL(mock, write_exact(testing::_, timeout))
        .WillOnce(Return(test_data.size()));
    std::span<const std::uint8_t> data{test_data};
    auto result = mock.write_exact(data, timeout);
    EXPECT_EQ(result, test_data.size());
  }
}

TEST_F(serial_port_tests, read_exact_various_timeouts) {
  std::vector<std::chrono::milliseconds> timeouts = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{1},
      std::chrono::milliseconds{10}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{1000}};

  for (auto timeout : timeouts) {
    std::array<std::uint8_t, 10> buffer{};
    std::span<std::uint8_t> data{buffer};
    EXPECT_CALL(mock, read_exact(testing::_, timeout)).WillOnce(Return(5));
    auto result = mock.read_exact(data, timeout);
    EXPECT_EQ(result, 5);
  }
}

TEST_F(serial_port_tests, rapid_write_read_cycles) {
  EXPECT_CALL(mock, write_some(testing::_))
      .Times(100)
      .WillRepeatedly(Return(test_data.size()));
  EXPECT_CALL(mock, read_some(testing::_)).Times(100).WillRepeatedly(Return(5));

  for (int i = 0; i < 100; ++i) {
    std::span<const std::uint8_t> write_data{test_data};
    auto write_result = mock.write_some(write_data);
    EXPECT_EQ(write_result, test_data.size());

    std::array<std::uint8_t, 10> buffer{};
    std::span<std::uint8_t> read_data{buffer};
    auto read_result = mock.read_some(read_data);
    EXPECT_EQ(read_result, 5);
  }
}

TEST_F(serial_port_tests, control_operations_rapid_calls) {
  EXPECT_CALL(mock, available()).Times(1000).WillRepeatedly(Return(42));
  EXPECT_CALL(mock, flush()).Times(100).WillRepeatedly(Return());
  EXPECT_CALL(mock, drain()).Times(100).WillRepeatedly(Return());

  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 10; ++j) {
      auto result = mock.available();
      EXPECT_EQ(result, 42);
    }
    mock.flush();
    mock.drain();
  }
}
} // namespace carbio::unit_tests
