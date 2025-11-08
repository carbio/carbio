#pragma once

#include "fingerprint/command_executor.h"
#include "fingerprint/command_code.h"
#include "fingerprint/command_traits.h"
#include "fingerprint/result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace carbio::unit_tests {
class command_executor_mock {
public:
  // Mock the execute method for all command types
  MOCK_METHOD((void_result),
              execute_capture_image,
              ((const typename command_traits<command_code::capture_image>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_extract_features,
              ((const typename command_traits<command_code::extract_features>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_merge_model,
              ((const typename command_traits<command_code::merge_model>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_store_model,
              ((const typename command_traits<command_code::store_model>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_load_model,
              ((const typename command_traits<command_code::load_model>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_erase_model,
              ((const typename command_traits<command_code::erase_model>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_clear_database,
              ((const typename command_traits<command_code::clear_database>::request&)),
              (noexcept));

  MOCK_METHOD((result<match_query_info>),
              execute_match_model,
              ((const typename command_traits<command_code::match_model>::request&)),
              (noexcept));

  MOCK_METHOD((result<search_query_info>),
              execute_search_model,
              ((const typename command_traits<command_code::search_model>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_turn_led_on,
              ((const typename command_traits<command_code::turn_led_on>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_turn_led_off,
              ((const typename command_traits<command_code::turn_led_off>::request&)),
              (noexcept));

  MOCK_METHOD((void_result),
              execute_soft_reset_device,
              ((const typename command_traits<command_code::soft_reset_device>::request&)),
              (noexcept));

  MOCK_METHOD((result<std::uint16_t>),
              execute_count_model,
              ((const typename command_traits<command_code::count_model>::request&)),
              (noexcept));

  MOCK_METHOD((result<device_settings>),
              execute_read_system_parameter,
              ((const typename command_traits<command_code::read_system_parameter>::request&)),
              (noexcept));

  MOCK_METHOD(void_result,
              send_data_packets,
              (std::span<const std::uint8_t> data),
              (noexcept));
};
} // namespace carbio::unit_tests
