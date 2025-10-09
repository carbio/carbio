#ifndef CARBIO_UNIT_SERIAL_PORT_MOCK_H
#define CARBIO_UNIT_SERIAL_PORT_MOCK_H

#include "carbio/serial_port.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace carbio::unit_tests
{
class serial_port_mock
{
public:
  MOCK_METHOD(bool, is_open, (), (const, noexcept));
  MOCK_METHOD(void, open, (std::string_view path, serial_config configuration), (noexcept));
  MOCK_METHOD(void, close, (), (noexcept));

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

#endif // CARBIO_UNIT_SERIAL_PORT_MOCK_H
