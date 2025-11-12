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

#include "fingerprint/fingerprint_sensor.h"

#include "fingerprint/command_serializer.h"

#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <thread>

namespace carbio
{
fingerprint_sensor::fingerprint_sensor()
    : serial_()
    , protocol_()
    , executor_(serial_, protocol_)
    , settings_()
{
}

fingerprint_sensor::~fingerprint_sensor() = default;

// --- connection management

bool fingerprint_sensor::is_connected() const noexcept
{
  return serial_.is_open();
}

bool fingerprint_sensor::connect(const char* path) noexcept
{
  static constexpr std::array<std::uint32_t, 12> baud_rates = {57600, 115200, 9600, 19200, 28800, 38400, 48000, 67200, 76800, 86400, 96000, 105600};
  SPDLOG_INFO("Attempting to connect sensor...");
  for (auto baud : baud_rates)
  {
    if (!serial_.open(path))
    {
      SPDLOG_ERROR("Failed to open port at {}", path);
      continue;
    }

    // Now set baud rate after port is open
    if (!serial_.set_baud_rate(baud))
    {
      SPDLOG_ERROR("Failed to set baud rate {}", baud);
      serial_.close();
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (auto r = verify_device_password(carbio::scoped_zero<std::uint32_t>(0x00000000)); !r)
    {
      SPDLOG_ERROR("Failed device password authentication");
      serial_.close();
      continue;
    }

    if (auto r = query_device_settings(); r)
    {
      SPDLOG_INFO("Connection established to sensor!");
      settings_ = r.value();
      protocol_.set_packet_length(settings_.length);
      return true;
    }
    else
    {
      SPDLOG_ERROR("Failed reading device settings");
      serial_.close();
      continue; // Try next baud rate
    }
  }

  SPDLOG_ERROR("Failed to connect sensor!");
  return false;
}

void fingerprint_sensor::disconnect() noexcept
{
  SPDLOG_INFO("Closing sensor connection...");
  serial_.close();
}

// --- device configuration

const device_settings& fingerprint_sensor::get_device_settings() const noexcept
{
  return settings_;
}

result<device_settings> fingerprint_sensor::query_device_settings() noexcept
{
  SPDLOG_TRACE("querrying device settings...");
  return executor_.execute<command_code::read_system_parameter>({});
}

void_result fingerprint_sensor::update_device_settings() noexcept
{
  auto r = query_device_settings();
  if (!r)
  {
    return make_error(r.error());
  }
  settings_ = r.value();
  return make_success();
}

void_result fingerprint_sensor::set_baud_rate_setting(baud_rate_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{static_cast<std::uint8_t>(device_setting_index::baud_rate_setting), static_cast<std::uint8_t>(setting)};
  SPDLOG_DEBUG("setting baud rate...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::set_security_level_setting(security_level_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{static_cast<std::uint8_t>(device_setting_index::security_level_setting), static_cast<std::uint8_t>(setting)};
  SPDLOG_DEBUG("setting security level...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::set_packet_length_setting(packet_length_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{static_cast<std::uint8_t>(device_setting_index::packet_length_setting), static_cast<std::uint8_t>(setting)};
  SPDLOG_DEBUG("setting packet data length...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::soft_reset_device() noexcept
{
  SPDLOG_DEBUG("performing soft reset...");
  return executor_.execute<command_code::soft_reset_device>({});
}

// --- led management

void_result fingerprint_sensor::set_led_setting(led_mode_setting mode, uint8_t speed, led_color_setting color, uint8_t cycles) noexcept
{
  SPDLOG_DEBUG("setting led configuration");
  return executor_.execute<command_code::set_led_config>({static_cast<std::uint8_t>(mode), speed, static_cast<std::uint8_t>(color), cycles});
}

void_result fingerprint_sensor::turn_led_on() noexcept
{
  SPDLOG_DEBUG("turning led on...");
  return executor_.execute<command_code::turn_led_on>({});
}

void_result fingerprint_sensor::turn_led_off() noexcept
{
  SPDLOG_DEBUG("turning led off...");
  return executor_.execute<command_code::turn_led_off>({});
}

// --- security

void_result fingerprint_sensor::set_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept
{
  SPDLOG_DEBUG("setting device password...");
  return executor_.execute<command_code::set_device_password>({std::move(password)});
}

void_result fingerprint_sensor::verify_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept
{
  SPDLOG_DEBUG("verifying device password...");
  return executor_.execute<command_code::verify_device_password>({std::move(password)});
}

void_result fingerprint_sensor::set_device_address(std::uint32_t new_address) noexcept
{
  protocol_.set_address(new_address);
  return make_success();
}

// --- low-level ops

void_result fingerprint_sensor::capture_image() noexcept
{
  SPDLOG_TRACE("capturing fingerprint image...");
  return executor_.execute<command_code::capture_image>({});
}

void_result fingerprint_sensor::extract_features(std::uint8_t buffer_id)
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  SPDLOG_TRACE("extracting feature points of the fingerprint image...");
  return executor_.execute<command_code::extract_features>(std::array<std::uint8_t, 1>{buffer_id});
}

void_result fingerprint_sensor::merge_model() noexcept
{
  SPDLOG_TRACE("merging feature points into template model...");
  return executor_.execute<command_code::merge_model>({});
}

void_result fingerprint_sensor::store_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  SPDLOG_TRACE("storing template model...");
  return executor_.execute<command_code::store_model>({buffer_id, page_id});
}

void_result fingerprint_sensor::load_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  SPDLOG_TRACE("loading template model...");
  return executor_.execute<command_code::load_model>({buffer_id, page_id});
}

void_result fingerprint_sensor::upload_model(std::span<const std::uint8_t> buffer, std::uint8_t buffer_id) noexcept
{

  auto result = executor_.execute<command_code::upload_model>({buffer_id});
  if (!result)
    return make_error(result.error());
  SPDLOG_TRACE("uploading template model...");
  return executor_.send_data_packets(buffer);
}

result<locked_buffer<std::uint8_t>> fingerprint_sensor::download_model(std::uint8_t buffer_id) noexcept
{
  // First send the download command
  SPDLOG_TRACE("downloading template model...");
  auto result = executor_.execute<command_code::download_model>({buffer_id});
  if (!result)
    return make_error(result.error());

  // Then receive data packets (returns locked_buffer)
  auto data_result = executor_.receive_data_packets();
  if (!data_result)
    return make_error(data_result.error());

  SPDLOG_TRACE("template model downloaded successfully ({} bytes)", data_result->size());
  return make_success(std::move(*data_result));
}

void_result fingerprint_sensor::upload_image(std::span<const std::uint8_t> data) noexcept
{
  auto result = executor_.execute<command_code::upload_image>({});
  if (!result)
    return make_error(result.error());
  SPDLOG_TRACE("uploading fingerprint image...");
  return executor_.send_data_packets(data);
}

result<locked_buffer<std::uint8_t>> fingerprint_sensor::download_image() noexcept
{
  // First send the download command
  SPDLOG_TRACE("downloading fingerprint image...");
  auto result = executor_.execute<command_code::download_image>({});
  if (!result)
    return make_error(result.error());

  // Then receive data packets (returns locked_buffer)
  auto data_result = executor_.receive_data_packets();
  if (!data_result)
    return make_error(data_result.error());

  SPDLOG_TRACE("fingerprint image downloaded successfully ({} bytes)", data_result->size());
  return make_success(std::move(*data_result));
}

// --- database ops

void_result fingerprint_sensor::erase_model(std::uint16_t page_id, std::uint16_t count) noexcept
{
  SPDLOG_TRACE("erasing template model...");
  return executor_.execute<command_code::erase_model>({page_id, count});
}

void_result fingerprint_sensor::clear_database() noexcept
{
  SPDLOG_TRACE("clearing database...");
  return executor_.execute<command_code::clear_database>({});
}

result<match_query_info> fingerprint_sensor::match_model() noexcept
{
  SPDLOG_TRACE("matching template model...");
  return executor_.execute<command_code::match_model>({});
}

result<search_query_info> fingerprint_sensor::search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  SPDLOG_TRACE("searching template model...");
  return executor_.execute<command_code::search_model>({buffer_id, page_id, count});
}

result<search_query_info> fingerprint_sensor::fast_search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  SPDLOG_TRACE("fast searching template model...");
  return executor_.execute<command_code::fast_search_model>({buffer_id, page_id, count});
}

result<std::uint16_t> fingerprint_sensor::model_count() noexcept
{
  SPDLOG_TRACE("view template model count...");
  return executor_.execute<command_code::count_model>({});
}

result<std::vector<std::uint8_t>> fingerprint_sensor::read_index_table(std::span<std::uint8_t> data) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  SPDLOG_DEBUG("reading index table...");
  auto result = executor_.execute<command_code::read_index_table>(std::array<std::uint8_t, 1>{0});
  if (!result)
    return make_error(result.error());

  // Copy to output buffer and return
  std::size_t copy_size = std::min(data.size(), result->size());
  std::copy_n(result->begin(), copy_size, data.begin());

  return make_success(std::vector<std::uint8_t>(result->begin(), result->end()));
}

void_result fingerprint_sensor::write_notepad(std::uint8_t page_number, std::span<const std::uint8_t, 32> data) noexcept
{
  // Prepare request: page_number + 32-byte data
  command_traits<command_code::write_notepad>::request req;
  req.page_number = page_number;
  std::copy(data.begin(), data.end(), req.data.begin());

  SPDLOG_DEBUG("writing notepad page {}...", page_number);
  return executor_.execute<command_code::write_notepad>(req);
}

result<std::array<std::uint8_t, 32>> fingerprint_sensor::read_notepad(std::uint8_t page_number) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  SPDLOG_DEBUG("reading notepad page {}...", page_number);
  // Use explicit array construction to avoid stack protector warning
  std::array<std::uint8_t, 1> request{page_number};
  return executor_.execute<command_code::read_notepad>(request);
}

void_result fingerprint_sensor::enroll(std::uint16_t position, std::uint16_t sample_count) noexcept
{
  if (sample_count < 2) // at least 2 samples needed
  {
    SPDLOG_ERROR("sample_count must be >= 2");
    return make_error(status_code::invalid_device_configuration);
  }

  if (sample_count < 8) // warn user that industry practice requires at least 8 samples
    SPDLOG_WARN("Using fewer than recommended samples (recommended >= 8) may reduce reliability.");

  SPDLOG_INFO("Enrolling fingerprint with {} samples...", sample_count);

  if (auto r = capture_and_extract(1); !r) // create the first sample
    return make_error(r.error());

  for (std::uint16_t s = 2; s <= sample_count; ++s) // go from 2 to N sample
  {
    if (auto r = capture_and_extract(2); !r) // capture and extract print into slot #2
      return make_error(r.error());

    if (auto r = merge_model(); !r) // puts merged sample into slot #1
      return make_error(r.error());
  }

  if (auto r = store_model(position, 1); !r) // store the merged print stored at slot #1
    return make_error(r.error());

  SPDLOG_INFO("Enrollment complete with {} samples", sample_count);

  return make_success();
}

void_result fingerprint_sensor::enroll_with_progress(std::uint16_t position, std::function<void(int, int, const char*)> callback, std::uint16_t sample_count) noexcept
{
  if (sample_count < 2) // at least 2 samples needed
  {
    SPDLOG_ERROR("sample_count must be >= 2");
    return make_error(status_code::invalid_device_configuration);
  }

  if (sample_count < 8) // warn user that industry practice requires at least 8 samples
    SPDLOG_WARN("Using fewer than recommended samples (recommended >= 8) may reduce reliability.");

  SPDLOG_INFO("Enrolling fingerprint with {} samples...", sample_count);

  if (auto r = capture_and_extract(1); !r) // create the first sample
    return make_error(r.error());

  callback(1, sample_count, "Sample 1 captured"); // fire the callback

  for (std::uint16_t i = 2; i <= sample_count; ++i) // go from 2 to N
  {
    if (auto r = capture_and_extract(2); !r) // create i-th sample
      return make_error(r.error());

    if (auto r = merge_model(); !r) // merge slot #1 and slot #2 into slot #1
      return make_error(r.error());

    callback(i, sample_count, "Sample captured"); // fire the callback
  }

  if (auto r = store_model(position, 1); !r) // flash merged print at slot #1
    return make_error(r.error());

  SPDLOG_INFO("Enrollment complete with {} samples", sample_count);
  callback(sample_count, sample_count, "Enrollment complete"); // fire the callback

  return make_success();
}

result<match_query_info> fingerprint_sensor::verify(std::uint16_t position, std::chrono::milliseconds timeout) noexcept
{
  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());

  if (auto r = load_model(position, 2); !r)
    return make_error(r.error());

  // Deadline-based retry with adaptive delay for device_busy status
  // Only sleeps when device is actually busy - zero overhead for fast operations
  const auto deadline = timeout;
  auto start_time = std::chrono::steady_clock::now();
  int attempt = 0;
  int delay_ms = 50;

  while (std::chrono::steady_clock::now() - start_time < deadline)
  {
    auto r = match_model();
    if (r || r.error() != status_code::device_busy)
      return r;

    // Only sleep if device_busy - adaptive delay: 50, 75, 100, 125, 150ms, etc. (capped at 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    attempt++;
    delay_ms = std::min(50 + (attempt * 25), 200);
  }

  // If still busy after deadline, return timeout
  SPDLOG_ERROR("Match operation timed out after {} attempts ({}ms elapsed)",
               attempt, std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start_time).count());
  return make_error(status_code::timeout);
}

result<match_query_info> fingerprint_sensor::verify_with_progress(std::uint16_t position, std::function<void(int, const char*)> callback, std::chrono::milliseconds timeout) noexcept
{
  callback(0, "Place finger on sensor");

  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  callback(33, "Fingerprint captured");

  if (auto r = load_model(position, 2); !r)
    return make_error(r.error());
  callback(66, "Template loaded");

  // Deadline-based retry with adaptive delay for device_busy status
  // Only sleeps when device is actually busy - zero overhead for fast operations
  const auto deadline = timeout;
  auto start_time = std::chrono::steady_clock::now();
  int attempt = 0;
  int delay_ms = 50;

  while (std::chrono::steady_clock::now() - start_time < deadline)
  {
    auto r = match_model();
    if (r || r.error() != status_code::device_busy)
    {
      if (r)
        callback(100, "Verification complete");
      return r;
    }

    // Only sleep if device_busy - adaptive delay: 50, 75, 100, 125, 150ms, etc. (capped at 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    attempt++;
    delay_ms = std::min(50 + (attempt * 25), 200);
  }

  // If still busy after deadline, return timeout
  SPDLOG_ERROR("Match operation timed out after {} attempts ({}ms elapsed)",
               attempt, std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start_time).count());
  return make_error(status_code::timeout);
}

result<search_query_info> fingerprint_sensor::identify(std::chrono::milliseconds timeout) noexcept
{
  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());

  // Deadline-based retry with adaptive delay for device_busy status
  // Only sleeps when device is actually busy - zero overhead for fast operations
  const auto deadline = timeout;
  auto start_time = std::chrono::steady_clock::now();
  int attempt = 0;
  int delay_ms = 50;

  while (std::chrono::steady_clock::now() - start_time < deadline)
  {
    auto r = fast_search_model(0, 1, settings_.capacity);
    if (r || r.error() != status_code::device_busy)
      return r;

    // Only sleep if device_busy - adaptive delay: 50, 75, 100, 125, 150ms, etc. (capped at 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    attempt++;
    delay_ms = std::min(50 + (attempt * 25), 200);
  }

  // If still busy after deadline, return timeout
  SPDLOG_ERROR("Search operation timed out after {} attempts ({}ms elapsed)",
               attempt, std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start_time).count());
  return make_error(status_code::timeout);
}

result<search_query_info> fingerprint_sensor::identify_with_progress(std::function<void(int, const char*)> callback, std::chrono::milliseconds timeout) noexcept
{
  callback(0, "Place finger on sensor");

  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  callback(50, "Fingerprint captured");

  // Deadline-based retry with adaptive delay for device_busy status
  // Only sleeps when device is actually busy - zero overhead for fast operations
  const auto deadline = timeout;
  auto start_time = std::chrono::steady_clock::now();
  int attempt = 0;
  int delay_ms = 50;

  while (std::chrono::steady_clock::now() - start_time < deadline)
  {
    auto r = fast_search_model(0, 1, settings_.capacity);
    if (r || r.error() != status_code::device_busy)
    {
      if (r)
        callback(100, "Search complete");
      return r;
    }

    // Only sleep if device_busy - adaptive delay: 50, 75, 100, 125, 150ms, etc. (capped at 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    attempt++;
    delay_ms = std::min(50 + (attempt * 25), 200);
  }

  // If still busy after deadline, return timeout
  SPDLOG_ERROR("Search operation timed out after {} attempts ({}ms elapsed)",
               attempt, std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start_time).count());
  return make_error(status_code::timeout);
}

void_result fingerprint_sensor::capture_and_extract(std::uint16_t position) noexcept
{
  SPDLOG_DEBUG("Place finger on sensor...");
  for (;;)
  {
    if (auto r = capture_image(); !r) // failed to capture
    {
      if (is_retryable(r.error()))
      {
        SPDLOG_TRACE("{}", message(r.error()));
        continue; // retry
      }
      SPDLOG_ERROR("{}", message(r.error()));
      return make_error(r.error()); // fatal: non-recoverable
    }
    break; // done
  }

  SPDLOG_DEBUG("Extracting features...");
  for (;;)
  {
    if (auto r = extract_features(position); !r) // failed to extract
    {
      if (is_retryable(r.error()))
      {
        SPDLOG_TRACE("{}", message(r.error()));
        continue; // retry
      }
      SPDLOG_ERROR("{}", message(r.error()));
      return make_error(r.error()); // fatal: non-recoverable
    }
    break; // done
  }
  // no errors
  SPDLOG_DEBUG("Remove finger...");

  // Limit retries to prevent infinite loop if sensor hangs
  // Each capture_image() has 1s timeout, so max ~10s total
  constexpr int max_removal_attempts = 10;

  for (int attempt = 0; attempt < max_removal_attempts; ++attempt)
  {
    if (auto r = capture_image(); !r)
    {
      if (status_code::no_finger == r.error())
      {
        SPDLOG_TRACE("Finger removed.");
        break;
      }
      if (!is_retryable(r.error()))
      {
        SPDLOG_ERROR("{}", message(r.error()));
        return make_error(r.error());
      }
      // Retryable error - continue loop
    }
    // If finger still present after max attempts, proceed anyway
    // This prevents infinite hang if sensor stops responding
    if (attempt == max_removal_attempts - 1)
    {
      SPDLOG_WARN("Finger removal detection timed out after {} attempts, proceeding anyway", max_removal_attempts);
    }
  }
  SPDLOG_DEBUG("Fingerprint captured.");
  return make_success();
}
} // namespace carbio
