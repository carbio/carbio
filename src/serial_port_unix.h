#ifndef CARBIO_SERIAL_PORT_UNIX_H
#define CARBIO_SERIAL_PORT_UNIX_H

#include "carbio/serial_port.h"
#include "descriptor.h"

#include <termios.h>

namespace carbio
{
class serial_port::impl
{
  serial_config   config_;
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
  bool        open(std::string_view path, serial_config config = {}) noexcept;
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
  void apply_config() noexcept;
  void apply_baud_rate(std::uint32_t baud_rate) noexcept;
  void apply_data_bits(std::uint8_t data_bits) noexcept;
  void apply_stop_bits(std::uint8_t stop_bits) noexcept;
  void apply_parity(std::uint8_t parity) noexcept;
  void apply_flow_control(std::uint8_t flow_control) noexcept;
  void apply_timeouts() noexcept;

  void store_port_state() noexcept;
  void restore_port_state() noexcept;
  void init_port_changes() noexcept;
  void apply_port_changes() noexcept;
};
} /* namespace carbio */

#endif // CARBIO_SERIAL_PORT_UNIX_H
