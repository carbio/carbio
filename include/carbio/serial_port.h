#ifndef CARBIO_SERIAL_PORT_H
#define CARBIO_SERIAL_PORT_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace carbio
{
enum class data_width : std::uint8_t
{
  _5 = 5,
  _6 = 6,
  _7 = 7,
  _8 = 8
};
enum class stop_width : std::uint8_t
{
  _1 = 1,
  _2 = 2
};
enum class parity_mode : std::uint8_t
{
  none = 1,
  odd  = 2,
  even = 3
};
enum class flow_control : std::uint8_t
{
  none     = 0,
  software = 1,
  hardware = 2,
  both     = 3
};

class serial_port
{
  struct impl;
  std::unique_ptr<impl> d;

public:
  serial_port() noexcept;
  ~serial_port() noexcept;
  serial_port(serial_port &&other) noexcept;
  serial_port &operator=(serial_port &&other) noexcept;
  serial_port(const serial_port &other)            = delete;
  serial_port &operator=(const serial_port &other) = delete;
  bool         is_open() const noexcept;
  bool         open(std::string_view device_path) noexcept;
  bool         set_baud_rate(std::uint32_t baud = 57600) noexcept;
  bool         set_data_width(data_width data = data_width::_8) noexcept;
  bool         set_stop_width(stop_width stop = stop_width::_1) noexcept;
  bool         set_parity_mode(parity_mode parity = parity_mode::none) noexcept;
  bool         set_flow_control(flow_control flow = flow_control::none) noexcept;
  //bool         set_blocking(bool value) noexcept;
  void         close() noexcept;
  std::size_t  write_some(std::span<const std::uint8_t> buffer) noexcept;
  std::size_t  read_some(std::span<std::uint8_t> buffer) noexcept;
  std::size_t  write_exact(std::span<const std::uint8_t> buffer,
                           std::chrono::milliseconds     timeout = std::chrono::milliseconds(1000)) noexcept;
  std::size_t  read_exact(std::span<std::uint8_t>   buffer,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) noexcept;

  std::size_t available() const noexcept;
  void        flush() noexcept;
  void        drain() noexcept;
  void        cancel() noexcept;
};
} /* namespace carbio */

#endif // CARBIO_SERIAL_PORT_H
