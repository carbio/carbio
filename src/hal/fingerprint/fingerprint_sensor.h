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

#include "fingerprint/baud_rate_setting.h"
#include "fingerprint/command_code.h"
#include "fingerprint/command_executor.h"
#include "fingerprint/device_info.h"
#include "fingerprint/device_setting_index.h"
#include "fingerprint/device_settings.h"
#include "fingerprint/led_color_setting.h"
#include "fingerprint/led_mode_setting.h"
#include "fingerprint/match_query_info.h"
#include "fingerprint/packet_length_setting.h"
#include "fingerprint/protocol_handler.h"
#include "fingerprint/result.h"
#include "fingerprint/search_query_info.h"
#include "fingerprint/security_level_setting.h"
#include "io/serial_port.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace carbio
{
class fingerprint_sensor
{
  serial_port serial_;
  protocol_handler protocol_;
  command_executor<serial_port, protocol_handler> executor_;
  device_settings settings_{};

public:
  // construction & destruction
  fingerprint_sensor();
  ~fingerprint_sensor();
  fingerprint_sensor(fingerprint_sensor&& other) noexcept = delete; // non-movable
  fingerprint_sensor& operator=(fingerprint_sensor&&) noexcept = delete;
  fingerprint_sensor(fingerprint_sensor const&) = delete; // non-copiable
  fingerprint_sensor& operator=(fingerprint_sensor const&) = delete;

  // --- connection management
  bool is_connected() const noexcept;
  bool connect(const char* path) noexcept;
  void disconnect() noexcept;

  // --- device configuration
  const device_settings& get_device_settings() const noexcept;
  result<device_settings> query_device_settings() noexcept;
  void_result update_device_settings() noexcept;
  void_result set_baud_rate_setting(baud_rate_setting setting) noexcept;
  void_result set_security_level_setting(security_level_setting setting) noexcept;
  void_result set_packet_length_setting(packet_length_setting setting) noexcept;
  void_result soft_reset_device() noexcept;

  // --- led management
  void_result set_led_setting(led_mode_setting mode, uint8_t speed = 128, led_color_setting color = led_color_setting::blue, uint8_t cycles = 0) noexcept;
  void_result turn_led_on() noexcept;
  void_result turn_led_off() noexcept;

  // --- security
  void_result set_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept;
  void_result verify_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept;
  void_result set_device_address(std::uint32_t new_address) noexcept;

  // --- low-level ops
  void_result capture_image() noexcept;
  void_result extract_features(std::uint8_t buffer_id = 1);
  void_result merge_model() noexcept;
  void_result store_model(std::uint16_t page_id, std::uint8_t buffer_id = 1) noexcept;
  void_result load_model(std::uint16_t page_id, std::uint8_t buffer_id = 1) noexcept;
  void_result upload_model(std::span<const std::uint8_t> buffer, std::uint8_t buffer_id = 1) noexcept;
  result<locked_buffer<std::uint8_t>> download_model(std::uint8_t buffer_id = 1) noexcept;

  // --- database ops
  void_result upload_image(std::span<const std::uint8_t> data) noexcept;
  result<locked_buffer<std::uint8_t>> download_image() noexcept;
  void_result erase_model(std::uint16_t page_id, std::uint16_t count = 1) noexcept;
  void_result clear_database() noexcept;
  result<match_query_info> match_model() noexcept;
  result<search_query_info> search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1, std::uint16_t count = 0) noexcept;
  result<search_query_info> fast_search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1, std::uint16_t count = 0) noexcept;
  result<std::uint16_t> model_count() noexcept;
  result<std::vector<std::uint8_t>> read_index_table(std::span<std::uint8_t> data) noexcept;

  // --- notepad ops
  void_result write_notepad(std::uint8_t page_number, std::span<const std::uint8_t, 32> data) noexcept;
  result<std::array<std::uint8_t, 32>> read_notepad(std::uint8_t page_number) noexcept;

  // --- convenience ops
  void_result enroll(std::uint16_t position, std::uint16_t sample_count = 12) noexcept;
  void_result enroll_with_progress(std::uint16_t position, std::function<void(int, int, const char*)> callback, std::uint16_t sample_count = 12) noexcept;
  result<match_query_info> verify(std::uint16_t position, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) noexcept;
  result<match_query_info> verify_with_progress(std::uint16_t position, std::function<void(int, const char*)> callback, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) noexcept;
  result<search_query_info> identify(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) noexcept;
  result<search_query_info> identify_with_progress(std::function<void(int, const char*)> callback, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) noexcept;

private:
  // -- utility ops
  void_result capture_and_extract(std::uint16_t position) noexcept;
};
} // namespace carbio
