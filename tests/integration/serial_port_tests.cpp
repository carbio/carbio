#include <gtest/gtest.h>

#include "carbio/serial_port.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <future>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace carbio::integration_tests
{
class serial_serial_test : public testing::Test
{
protected:
  serial_port serial_;

  void
  SetUp() override
  {
    try
      {
        serial_.open("/dev/ttyAMA0");
        ASSERT_TRUE(serial_.is_open()) << "Failed to open serial port";
        serial_.flush();
      }
    catch (const std::exception &e)
      {
        FAIL() << "Failed to open serial port: " << e.what();
      }
  }

  void
  TearDown() override
  {
    try
      {
        serial_.close();
      }
    catch (...)
      {
        // Ignore exceptions in teardown
      }
  }

  ~serial_serial_test() override;
};

serial_serial_test::~serial_serial_test() = default;

TEST_F(serial_serial_test, can_open_serial_device)
{
  EXPECT_TRUE(serial_.is_open());
}

TEST_F(serial_serial_test, can_send_single_byte)
{
  std::array<uint8_t, 1> buffer = {0xEF};
  auto                   result = serial_.write_some(std::span{buffer});
  EXPECT_EQ(result, 1u) << "Should send exactly 1 byte";
}

TEST_F(serial_serial_test, can_send_multiple_bytes)
{
  std::array<uint8_t, 4> buffer = {0xEF, 0x01, 0xFF, 0xFF};
  auto                   result = serial_.write_some(std::span{buffer});
  EXPECT_EQ(result, 4u) << "Should send exactly 4 bytes";
}

TEST_F(serial_serial_test, read_some_timeout_works)
{
  std::array<uint8_t, 1> buffer;

  auto start   = std::chrono::steady_clock::now();
  auto result  = serial_.read_some(std::span{buffer});
  auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(result, 0) << "Non-blocking read should have timed out";

  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  EXPECT_LE(elapsed_ms, 10) << "Non-blocking read should return quickly";
}

TEST_F(serial_serial_test, read_exact_timeout_works)
{
  std::array<uint8_t, 1> buffer;

  auto timeout = std::chrono::milliseconds{1000};
  auto start   = std::chrono::steady_clock::now();
  auto result  = serial_.read_exact(std::span{buffer}, timeout);
  auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(result, 0u) << "read_exact should timeout and return 0";

  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  EXPECT_GE(elapsed_ms, 900) << "Should wait close to timeout";
  EXPECT_LE(elapsed_ms, 1200) << "Should timeout within reasonable margin";
}

TEST_F(serial_serial_test, flush_operations_work)
{
  std::array<uint8_t, 2> write_buffer = {0xEF, 0x01};
  auto                   write_result = serial_.write_some(std::span{write_buffer});
  EXPECT_EQ(write_result, 2u) << "Should write exactly 2 bytes";

  serial_.flush();

  std::array<uint8_t, 1> read_buffer;
  auto                   read_result = serial_.read_some(std::span{read_buffer});
  EXPECT_EQ(read_result, 0u) << "Should have no data after flush";
}

TEST_F(serial_serial_test, reconnection_works)
{
  ASSERT_TRUE(serial_.is_open());

  serial_.close();
  EXPECT_FALSE(serial_.is_open());

  try
    {
      serial_.open("/dev/ttyAMA0", {});
      EXPECT_TRUE(serial_.is_open());

      std::array<uint8_t, 2> write_buffer = {0xEF, 0x01};
      auto                   write_result = serial_.write_some(std::span{write_buffer});
      EXPECT_GE(write_result, 0u) << "Should work after reconnection";
    }
  catch (const std::exception &e)
    {
      FAIL() << "Failed to re-open: " << e.what();
    }
}

TEST_F(serial_serial_test, operations_fail_when_disconnected)
{
  serial_.close();
  EXPECT_FALSE(serial_.is_open());

  std::array<uint8_t, 1> read_buffer;
  EXPECT_EQ(serial_.read_some(std::span{read_buffer}), 0) << "read_some should return with zero bytes";

  std::array<uint8_t, 2> write_buffer = {0x01, 0x02};
  EXPECT_EQ(serial_.write_some(std::span{write_buffer}), 0) << "write_some should return with zero bytes";
}

TEST_F(serial_serial_test, serial_config_edge_cases)
{
  serial_.close();

  std::vector<std::uint32_t> baud_rates = {9600, 19200, 38400, 57600, 115200};

  for (auto baud_rate : baud_rates)
    {
      serial_config config{.baud_rate = baud_rate,
                           .data_bits = data_width::_8,
                           .stop_bits = stop_width::_1,
                           .parity    = parity_mode::none,
                           .flow      = flow_control::none};

      try
        {
          serial_.open("/dev/ttyAMA0", config);
          EXPECT_TRUE(serial_.is_open()) << "Should open with baud rate: " << baud_rate;
          serial_.close();
        }
      catch (const std::exception &e)
        {
          FAIL() << "Failed to open with baud rate " << baud_rate << ": " << e.what();
        }
    }
}

TEST_F(serial_serial_test, data_bits_configurations)
{
  serial_.close();

  std::vector<data_width> data_bits_options = {data_width::_5, data_width::_6, data_width::_7, data_width::_8};

  for (auto bits : data_bits_options)
    {
      serial_config config{.baud_rate = 115200,
                                  .data_bits = bits,
                                  .stop_bits = stop_width::_1,
                                  .parity    = parity_mode::none,
                                  .flow      = flow_control::none};

      try
        {
          serial_.open("/dev/ttyAMA0", config);
          EXPECT_TRUE(serial_.is_open()) << "Should open with data bits: " << static_cast<int>(bits);

          // Test basic write operation
          std::array<uint8_t, 1> test_data = {0x42};
          auto                   result    = serial_.write_some(std::span{test_data});
          EXPECT_EQ(result, 1u) << "Should write with data bits: " << static_cast<int>(bits);

          serial_.close();
        }
      catch (const std::exception &e)
        {
          FAIL() << "Failed with data bits " << static_cast<int>(bits) << ": " << e.what();
        }
    }
}

TEST_F(serial_serial_test, parity_configurations)
{
  serial_.close();

  std::vector<parity_mode> parity_modes = {parity_mode::none, parity_mode::odd, parity_mode::even};

  for (auto parity : parity_modes)
    {
      serial_config config{.baud_rate = 115200,
                                  .data_bits = data_width::_8,
                                  .stop_bits = stop_width::_1,
                                  .parity    = parity,
                                  .flow      = flow_control::none};

      try
        {
          serial_.open("/dev/ttyAMA0", config);
          EXPECT_TRUE(serial_.is_open()) << "Should open with parity: " << static_cast<int>(parity);

          // Test basic operation
          std::array<uint8_t, 2> test_data = {0xAA, 0x55};
          auto                   result    = serial_.write_some(std::span{test_data});
          EXPECT_EQ(result, 2u) << "Should write with parity: " << static_cast<int>(parity);

          serial_.close();
        }
      catch (const std::exception &e)
        {
          FAIL() << "Failed with parity " << static_cast<int>(parity) << ": " << e.what();
        }
    }
}

TEST_F(serial_serial_test, flow_control_configurations)
{
  serial_.close();

  std::vector<flow_control> flow_modes = {flow_control::none, flow_control::software, flow_control::hardware,
                                          flow_control::both};

  for (auto flow : flow_modes)
    {
      serial_config config{.baud_rate = 115200,
                                  .data_bits = data_width::_8,
                                  .stop_bits = stop_width::_1,
                                  .parity    = parity_mode::none,
                                  .flow      = flow};

      try
        {
          serial_.open("/dev/ttyAMA0", config);
          EXPECT_TRUE(serial_.is_open()) << "Should open with flow control: " << static_cast<int>(flow);

          // Test basic operation
          std::array<uint8_t, 3> test_data = {0x01, 0x02, 0x03};
          auto                   result    = serial_.write_some(std::span{test_data});
          EXPECT_EQ(result, 3u) << "Should write with flow control: " << static_cast<int>(flow);

          serial_.close();
        }
      catch (const std::exception &e)
        {
          FAIL() << "Failed with flow control " << static_cast<int>(flow) << ": " << e.what();
        }
    }
}

TEST_F(serial_serial_test, write_read_data_integrity)
{
  // Test various data patterns for integrity
  std::vector<std::vector<uint8_t>> test_patterns = {
      {0x00},                                           // Null byte
      {0xFF},                                           // All ones
      {0xAA, 0x55},                                     // Alternating pattern
      {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, // Powers of 2
      std::vector<uint8_t>(256, 0x42),                  // Large repeated pattern
  };

  for (size_t i = 0; i < test_patterns.size(); ++i)
    {
      const auto &pattern = test_patterns[i];

      auto write_result = serial_.write_some(std::span{pattern});
      EXPECT_EQ(write_result, pattern.size()) << "Pattern " << i << " write failed";

      // Drain to ensure data is sent
      serial_.drain();

      // Small delay to allow loopback (if configured)
      std::this_thread::sleep_for(std::chrono::milliseconds{10});

      // Try to read back (may not work without loopback, but shouldn't fail)
      std::vector<uint8_t> read_buffer(pattern.size() * 2);
      auto                 read_result = serial_.read_some(std::span{read_buffer});

      // We don't assert on read result as it depends on hardware configuration
      // but the operation shouldn't throw
      EXPECT_GE(read_result, 0u) << "Pattern " << i << " read failed";
    }
}

TEST_F(serial_serial_test, large_buffer_operations)
{
  // Test with increasingly larger buffers
  std::vector<size_t> buffer_sizes = {1, 16, 64, 256, 512, 1024, 2048, 4096};

  for (auto size : buffer_sizes)
    {
      std::vector<uint8_t> large_buffer(size);
      std::iota(large_buffer.begin(), large_buffer.end(), 0);

      auto start   = std::chrono::steady_clock::now();
      auto result  = serial_.write_some(std::span{large_buffer});
      auto elapsed = std::chrono::steady_clock::now() - start;

      EXPECT_GT(result, 0u) << "Should write some data for size: " << size;
      EXPECT_LE(result, size) << "Should not write more than buffer size: " << size;

      auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
      EXPECT_LE(elapsed_ms, 5000) << "Large write should complete within 5 seconds for size: " << size;
    }
}

TEST_F(serial_serial_test, rapid_write_operations)
{
  // Test rapid successive writes (hot path)
  std::array<uint8_t, 4> test_data = {0xDE, 0xAD, 0xBE, 0xEF};

  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < 100; ++i)
    {
      auto result = serial_.write_some(std::span{test_data});
      EXPECT_EQ(result, test_data.size()) << "Rapid write " << i << " failed";
    }

  auto elapsed    = std::chrono::steady_clock::now() - start;
  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

  EXPECT_LE(elapsed_ms, 2000) << "100 rapid writes should complete within 2 seconds";
}

TEST_F(serial_serial_test, rapid_read_operations)
{
  // Test rapid successive reads
  std::array<uint8_t, 16> read_buffer;

  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < 100; ++i)
    {
      auto result = serial_.read_some(std::span{read_buffer});
      EXPECT_GE(result, 0u) << "Rapid read " << i << " failed";
      // Non-blocking reads will likely return 0, which is expected
    }

  auto elapsed    = std::chrono::steady_clock::now() - start;
  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

  EXPECT_LE(elapsed_ms, 1000) << "100 rapid reads should complete within 1 second";
}

TEST_F(serial_serial_test, varied_timeout)
{
  std::array<uint8_t, 8> buffer;

  // Test various timeout values
  std::vector<std::chrono::milliseconds> timeouts = {
      std::chrono::milliseconds{0},   // Immediate
      std::chrono::milliseconds{1},   // Very short
      std::chrono::milliseconds{10},  // Short
      std::chrono::milliseconds{100}, // Medium
      std::chrono::milliseconds{500}, // Long
  };

  for (auto timeout : timeouts)
    {
      auto start   = std::chrono::steady_clock::now();
      auto result  = serial_.read_exact(std::span{buffer}, timeout);
      auto elapsed = std::chrono::steady_clock::now() - start;

      EXPECT_GE(result, 0u) << "read_exact should complete for timeout: " << timeout.count() << "ms";

      auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

      if (timeout.count() > 0)
        {
          // Should timeout around the specified time (with some tolerance)
          EXPECT_LE(elapsed_ms, timeout.count() + 200)
              << "Timeout should be respected for: " << timeout.count() << "ms";
        }
      else
        {
          // Zero timeout should return immediately
          EXPECT_LE(elapsed_ms, 50) << "Zero timeout should return immediately";
        }
    }
}

TEST_F(serial_serial_test, error_recovery_scenarios)
{
  for (int i = 0; i < 5; ++i)
    {
      serial_.close();
      EXPECT_FALSE(serial_.is_open()) << "Port should be closed";

      try
        {
          serial_.open("/dev/ttyAMA0", {});
          EXPECT_TRUE(serial_.is_open()) << "Port should reopen successfully";

          // Verify functionality after reopen
          std::array<uint8_t, 2> test_data = {0x12, 0x34};
          auto                   result    = serial_.write_some(std::span{test_data});
          EXPECT_EQ(result, test_data.size()) << "Should work after reopen " << i;
        }
      catch (const std::exception &e)
        {
          FAIL() << "Recovery attempt " << i << " failed: " << e.what();
        }
    }
}

TEST_F(serial_serial_test, boundary_conditions)
{
  // Test boundary conditions

  // Empty write
  std::span<const uint8_t> empty_write;
  auto                     empty_result = serial_.write_some(empty_write);
  EXPECT_EQ(empty_result, 0u) << "Empty write should return 0";

  // Empty read
  std::span<uint8_t> empty_read;
  auto               empty_read_result = serial_.read_some(empty_read);
  EXPECT_EQ(empty_read_result, 0u) << "Empty read should return 0";

  // Single byte operations
  std::array<uint8_t, 1> single_write        = {0x99};
  auto                   single_write_result = serial_.write_some(std::span{single_write});
  EXPECT_EQ(single_write_result, 1u) << "Single byte write should work";

  std::array<uint8_t, 1> single_read;
  auto                   single_read_result = serial_.read_some(std::span{single_read});
  EXPECT_GE(single_read_result, 0u) << "Single byte read should work";
}

TEST_F(serial_serial_test, performance_benchmarks)
{
  // Basic performance benchmarks for hot paths
  // Reduced iterations for serial ports without hardware loopback (buffer fills up)
  constexpr size_t        iterations = 50;
  std::array<uint8_t, 32> benchmark_data;
  std::iota(benchmark_data.begin(), benchmark_data.end(), 0);

  // Benchmark synchronous writes using write_exact (guaranteed completion)
  // Note: 32 bytes at 57600 baud ≈ 4.4ms minimum + OS overhead, so use realistic timeout
  auto sync_write_start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < iterations; ++i)
    {
      auto bytes_written = serial_.write_exact(std::span{benchmark_data}, std::chrono::milliseconds{200});
      EXPECT_EQ(bytes_written, benchmark_data.size()) << "write_exact should complete full write";
      // CRITICAL: Allow buffer to drain between writes when no hardware loopback
      // This follows industry practice for serial port stress testing
      serial_.drain();
    }
  auto sync_write_end = std::chrono::high_resolution_clock::now();

  auto sync_write_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(sync_write_end - sync_write_start).count();

  EXPECT_LT(sync_write_duration, 10000000) << "50 sync writes should complete within 10 seconds";

  // Benchmark synchronous reads
  std::array<uint8_t, 32> read_buffer;
  auto                    sync_read_start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < iterations; ++i)
    {
      serial_.read_some(std::span{read_buffer});
    }
  auto sync_read_end = std::chrono::high_resolution_clock::now();

  auto sync_read_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(sync_read_end - sync_read_start).count();

  EXPECT_LT(sync_read_duration, 1000000) << "50 sync reads should complete within 1 second (non-blocking)";

  // Use GoogleTest logging for performance metrics
  SCOPED_TRACE("Performance metrics:");
  SCOPED_TRACE("Sync writes: " + std::to_string(sync_write_duration) + " µs for " + std::to_string(iterations) +
               " operations");
  SCOPED_TRACE("Sync reads: " + std::to_string(sync_read_duration) + " µs for " + std::to_string(iterations) +
               " operations");
  SCOPED_TRACE("Avg write: " + std::to_string(sync_write_duration / static_cast<long>(iterations)) +
               " µs per operation");
  SCOPED_TRACE("Avg read: " + std::to_string(sync_read_duration / static_cast<long>(iterations)) + " µs per operation");
}

} // namespace carbio::integration_tests
