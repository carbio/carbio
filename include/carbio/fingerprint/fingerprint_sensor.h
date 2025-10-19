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

#include "carbio/fingerprint/baud_rate_setting.h"
#include "carbio/fingerprint/command_code.h"
#include "carbio/fingerprint/command_executor.h"
#include "carbio/fingerprint/device_info.h"
#include "carbio/fingerprint/device_setting_index.h"
#include "carbio/fingerprint/device_setting_info.h"
#include "carbio/fingerprint/led_color_setting.h"
#include "carbio/fingerprint/led_mode_setting.h"
#include "carbio/fingerprint/match_query_info.h"
#include "carbio/fingerprint/packet_data_length_setting.h"
#include "carbio/fingerprint/protocol_handler.h"
#include "carbio/fingerprint/result.h"
#include "carbio/fingerprint/search_query_info.h"
#include "carbio/fingerprint/security_level_setting.h"
#include "carbio/io/serial_port.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace carbio::fingerprint {
class fingerprint_sensor {
  io::serial_port serial_;
  protocol_handler protocol_;
  command_executor executor_;

public:
  // --- construction / destruction / assignment

  fingerprint_sensor();
  ~fingerprint_sensor();
  fingerprint_sensor(fingerprint_sensor &&other) noexcept;
  fingerprint_sensor &operator=(fingerprint_sensor &&other) noexcept;
  fingerprint_sensor(fingerprint_sensor const &) = delete;
  fingerprint_sensor &operator=(fingerprint_sensor const &) = delete;

  // --- connection management
  [[nodiscard]] bool is_open() const noexcept;
  bool open(const char *path) noexcept;
  void close() noexcept;

  // --- device configuration

  [[nodiscard]] result<device_setting_info> get_device_setting_info() noexcept;
  [[nodiscard]] void_result
  set_baud_rate_setting(baud_rate_setting setting) noexcept;
  [[nodiscard]] void_result
  set_security_level_setting(security_level_setting setting) noexcept;
  [[nodiscard]] void_result
  set_packet_data_length_setting(packet_data_length_setting setting) noexcept;
  [[nodiscard]] void_result soft_reset_device() noexcept;

  // --- led management
  [[nodiscard]] void_result
  set_led_setting(led_mode_setting mode, uint8_t speed = 128,
                  led_color_setting color = led_color_setting::blue,
                  uint8_t cycles = 0) noexcept;
  [[nodiscard]] void_result turn_led_on() noexcept;
  [[nodiscard]] void_result turn_led_off() noexcept;

  // --- security
  [[nodiscard]] void_result
  set_device_password(carbio::secure_value<std::uint32_t> password) noexcept;
  [[nodiscard]] void_result
  verify_device_password(carbio::secure_value<std::uint32_t> password) noexcept;
  [[nodiscard]] void_result
  set_device_address(std::uint32_t new_address) noexcept;

  // --- low-level ops
  [[nodiscard]] void_result capture_image() noexcept;
  [[nodiscard]] void_result extract_features(std::uint8_t buffer_id = 1);
  [[nodiscard]] void_result create_model() noexcept;
  [[nodiscard]] void_result store_model(std::uint16_t page_id,
                                        std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] void_result load_model(std::uint16_t page_id,
                                       std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] void_result upload_model(std::span<const std::uint8_t> buffer,
                                         std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] result<locked_buffer<std::uint8_t>>
  download_model(std::uint8_t buffer_id = 1) noexcept;

  // --- database ops
  [[nodiscard]] void_result
  upload_image(std::span<const std::uint8_t> data) noexcept;
  [[nodiscard]] result<locked_buffer<std::uint8_t>>
  download_image() noexcept;
  [[nodiscard]] void_result erase_model(std::uint16_t page_id,
                                        std::uint16_t count = 1) noexcept;
  [[nodiscard]] void_result clear_database() noexcept;
  [[nodiscard]] result<match_query_info> match_model() noexcept;
  [[nodiscard]] result<search_query_info>
  search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1,
               std::uint16_t count = 0) noexcept;
  [[nodiscard]] result<search_query_info>
  fast_search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1,
                    std::uint16_t count = 0) noexcept;
  [[nodiscard]] result<std::uint16_t> model_count() noexcept;
  [[nodiscard]] result<std::vector<std::uint8_t>>
  read_index_table(std::span<std::uint8_t> data) noexcept;
};
} // namespace carbio::fingerprint
