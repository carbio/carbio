#pragma once

#include "carbio/io/serial_port.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace carbio::unit_tests
{
class serial_port_mock
{
public:
  MOCK_METHOD(bool, is_open, (), (const, noexcept));
  MOCK_METHOD(bool, open, (std::string_view path), (noexcept));
  MOCK_METHOD(void, close, (), (noexcept));
  MOCK_METHOD(bool, set_baud_rate, (std::uint32_t baud), (noexcept));
  MOCK_METHOD(bool, set_data_width, (io::data_width data), (noexcept));
  MOCK_METHOD(bool, set_stop_width, (io::stop_width stop), (noexcept));
  MOCK_METHOD(bool, set_parity_mode, (io::parity_mode parity), (noexcept));
  MOCK_METHOD(bool, set_flow_control, (io::flow_control flow), (noexcept));
  MOCK_METHOD(std::size_t, write_some, (std::span<const std::uint8_t> data), (noexcept));
  MOCK_METHOD(std::size_t, read_some, (std::span<std::uint8_t> data), (noexcept));
  MOCK_METHOD(std::size_t, write_exact, (std::span<const std::uint8_t> data, std::chrono::milliseconds timeout), (noexcept));
  MOCK_METHOD(std::size_t, read_exact, (std::span<std::uint8_t> data, std::chrono::milliseconds timeout), (noexcept));
  MOCK_METHOD(std::size_t, available, (), (const, noexcept));
  MOCK_METHOD(void, flush, (), (noexcept));
  MOCK_METHOD(void, drain, (), (noexcept));
  MOCK_METHOD(void, cancel, (), (noexcept));
};
} // namespace carbio::unit_tests
