#ifndef CARBIO_SERIAL_PORT_UNIX_H
#define CARBIO_SERIAL_PORT_UNIX_H

#include "carbio/serial_port.h"
#include "descriptor.h"

#include <termios.h>

namespace carbio
{
class serial_port::impl
{
  struct termios  newtty_;
  struct termios  oldtty_;
  file_descriptor fd_;

public:
  impl(impl &&other) noexcept            = default;
  impl &operator=(impl &&other) noexcept = default;
  impl(const impl &other)                = delete;
  impl &operator=(const impl &other)     = delete;
  impl() noexcept                        = default;
  ~impl() noexcept                       = default;
  bool        is_open() const noexcept;
  bool        open(std::string_view path) noexcept;
  bool        set_baud_rate(std::uint32_t baud = 57600) noexcept;
  bool        set_data_width(data_width data = data_width::_8) noexcept;
  bool        set_stop_width(stop_width stop = stop_width::_1) noexcept;
  bool        set_parity_mode(parity_mode parity = parity_mode::none) noexcept;
  bool        set_flow_control(flow_control flow = flow_control::none) noexcept;
  //bool        set_blocking(bool value) noexcept;
  void        close() noexcept;
  std::size_t write_some(std::span<const std::uint8_t> buffer) noexcept;
  std::size_t read_some(std::span<std::uint8_t> buffer) noexcept;
  std::size_t write_exact(std::span<const std::uint8_t> buffer,
                          std::chrono::milliseconds     timeout = std::chrono::milliseconds(1000)) noexcept;
  std::size_t read_exact(std::span<std::uint8_t>   buffer,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) noexcept;
  std::size_t available() const noexcept;
  void        flush() noexcept;
  void        drain() noexcept;
  void        cancel() noexcept;

private:
  bool restore_port_settings() noexcept;
  bool apply_port_settings() noexcept;
};
} /* namespace carbio */

#endif // CARBIO_SERIAL_PORT_UNIX_H
