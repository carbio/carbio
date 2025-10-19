/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#pragma once

#include "carbio/io/handle.h"

#include <termios.h>

#include <chrono>
#include <cstdint>
#include <span>

namespace carbio::io {
enum class data_width : std::uint8_t { _5 = 5, _6 = 6, _7 = 7, _8 = 8 };
enum class stop_width : std::uint8_t { _1 = 1, _2 = 2 };
enum class parity_mode : std::uint8_t { none = 1, odd = 2, even = 3 };
enum class flow_control : std::uint8_t {
  none = 0,
  software = 1,
  hardware = 2,
  both = 3
};

class serial_port {
  struct termios newtty_;
  struct termios oldtty_;
  unique_handle handle_;

public:
  serial_port(serial_port &&other) noexcept = default;
  serial_port &operator=(serial_port &&other) noexcept = default;
  serial_port(const serial_port &other) = delete;
  serial_port &operator=(const serial_port &other) = delete;
  serial_port() noexcept = default;
  ~serial_port() noexcept = default;
  bool is_open() const noexcept;
  bool open(const char *path) noexcept;
  bool set_baud_rate(std::uint32_t baud = 57600) noexcept;
  bool set_data_width(data_width data = data_width::_8) noexcept;
  bool set_stop_width(stop_width stop = stop_width::_1) noexcept;
  bool set_parity_mode(parity_mode parity = parity_mode::none) noexcept;
  bool set_flow_control(flow_control flow = flow_control::none) noexcept;
  void close() noexcept;
  std::size_t write_some(std::span<const std::uint8_t> buffer) noexcept;
  std::size_t read_some(std::span<std::uint8_t> buffer) noexcept;
  std::size_t write_exact(std::span<const std::uint8_t> buffer,
                          std::chrono::milliseconds timeout =
                              std::chrono::milliseconds(1000)) noexcept;
  std::size_t read_exact(std::span<std::uint8_t> buffer,
                         std::chrono::milliseconds timeout =
                             std::chrono::milliseconds(1000)) noexcept;
  std::size_t available() const noexcept;
  void flush() noexcept;
  void drain() noexcept;
  void cancel() noexcept;

private:
  bool restore_port_settings() noexcept;
  bool apply_port_settings() noexcept;
};
} /* namespace carbio::io */
