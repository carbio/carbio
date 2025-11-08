#pragma once

#include "fingerprint/protocol_handler.h"
#include "fingerprint/command_code.h"
#include "fingerprint/result.h"
#include "utility/locked_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <vector>

namespace carbio::unit_tests {
class protocol_handler_mock {
public:
  MOCK_METHOD(void, set_address, (std::uint32_t new_address), (noexcept));
  MOCK_METHOD(std::uint32_t, get_address, (), (const, noexcept));
  MOCK_METHOD(void, set_packet_length, (std::uint16_t new_length), (noexcept));
  MOCK_METHOD(std::uint16_t, get_packet_length, (), (const, noexcept));

  MOCK_METHOD(result<locked_buffer<std::uint8_t>>,
              construct_command_packet,
              (command_code code, std::span<const std::uint8_t> data),
              (const, noexcept));

  MOCK_METHOD(result<locked_buffer<std::uint8_t>>,
              parse_acknowledge_packet,
              (std::span<const std::uint8_t> data),
              (const, noexcept));

  MOCK_METHOD(result<std::vector<locked_buffer<std::uint8_t>>>,
              construct_data_packet,
              (std::span<const std::uint8_t> data),
              (const, noexcept));

  MOCK_METHOD(result<locked_buffer<std::uint8_t>>,
              parse_data_packet,
              (std::span<const std::span<std::uint8_t>> data),
              (const, noexcept));
};
} // namespace carbio::unit_tests
