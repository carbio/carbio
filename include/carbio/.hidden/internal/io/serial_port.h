#ifndef CARBIO_IO_SERIAL_PORT_H
#define CARBIO_IO_SERIAL_PORT_H

#include "carbio/internal/io/event_loop.h"

#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

namespace carbio::internal
{

enum class parity
{
  none,
  even,
  odd
};

enum class stop_bits
{
  one,
  two
};

enum class data_bits
{
  five  = 5,
  six   = 6,
  seven = 7,
  eight = 8
};

struct serial_config
{
  int32_t    baud_rate{57600};
  data_bits  data_bits_val{data_bits::eight};
  parity     parity_val{parity::none};
  stop_bits  stop_bits_val{stop_bits::one};
  bool       hardware_flow_control{false};
  bool       low_latency{true}; // Enable for RPi5 RP1 chip
};

class serial_port
{
public:
  explicit serial_port(event_loop &loop);
  ~serial_port();

  serial_port(serial_port const &)            = delete;
  serial_port &operator=(serial_port const &) = delete;
  serial_port(serial_port &&)                 = delete;
  serial_port &operator=(serial_port &&)      = delete;

  // Open serial port with configuration
  [[nodiscard]] std::expected<void, std::error_code> open(std::string_view device_path, serial_config const &config);

  // Close serial port
  void close();

  // Check if port is open
  [[nodiscard]] bool is_open() const noexcept
  {
    return m_fd >= 0;
  }

  // Async read exact number of bytes with timeout
  [[nodiscard]] auto read_exact(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
      -> std::expected<size_t, std::error_code>;

  // Async write data
  [[nodiscard]] auto write(std::span<const uint8_t> data) -> std::expected<size_t, std::error_code>;

  // Get file descriptor (for advanced use)
  [[nodiscard]] int fd() const noexcept
  {
    return m_fd;
  }

private:
  event_loop &m_loop;
  int         m_fd{-1};
  bool        m_registered{false};
};

// Awaitable for async read operation
class read_awaitable
{
public:
  read_awaitable(serial_port &port, event_loop &loop, std::span<uint8_t> buffer, std::chrono::milliseconds timeout);

  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<> handle);
  std::expected<size_t, std::error_code> await_resume();

private:
  void perform_read();
  void on_readable();
  void on_timeout();

  serial_port                              &m_port;
  event_loop                               &m_loop;
  std::span<uint8_t>                        m_buffer;
  std::chrono::milliseconds                 m_timeout;
  size_t                                    m_bytes_read{0};
  std::coroutine_handle<>                   m_handle{};
  int                                       m_timer_fd{-1};
  std::error_code                           m_error{};
  std::chrono::steady_clock::time_point     m_start_time{};
};

// Awaitable for async write operation
class write_awaitable
{
public:
  write_awaitable(serial_port &port, event_loop &loop, std::span<const uint8_t> data);

  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<> handle);
  std::expected<size_t, std::error_code> await_resume();

private:
  void perform_write();
  void on_writable();

  serial_port                  &m_port;
  event_loop                   &m_loop;
  std::span<const uint8_t>      m_data;
  size_t                        m_bytes_written{0};
  std::coroutine_handle<>       m_handle{};
  std::error_code               m_error{};
};

} // namespace carbio::internal

#endif // CARBIO_IO_SERIAL_PORT_H
