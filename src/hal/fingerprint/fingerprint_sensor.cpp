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
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
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
  spdlog::info("Attempting to connect sensor...");
  for (auto baud : baud_rates)
  {
    if (!serial_.open(path))
    {
      spdlog::error("Failed to open port at {}", path);
      continue;
    }

    // Now set baud rate after port is open
    if (!serial_.set_baud_rate(baud))
    {
      spdlog::error("Failed to set baud rate {}", baud);
      serial_.close();
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (auto r = verify_device_password(carbio::scoped_zero<std::uint32_t>(0x00000000)); !r)
    {
      spdlog::error("Failed device password authentication");
      serial_.close();
      continue;
    }

    if (auto r = query_device_settings(); r)
    {
      spdlog::info("Connection established to sensor!");
      settings_ = r.value();
      protocol_.set_packet_length(settings_.length);
      return true;
    }
    else
    {
      spdlog::error("Failed reading device settings");
      serial_.close();
      continue; // Try next baud rate
    }
  }

  spdlog::error("Failed to connect sensor!");
  return false;
}

void fingerprint_sensor::disconnect() noexcept
{
  spdlog::info("Closing sensor connection...");
  serial_.close();
}

// --- device configuration

const device_settings& fingerprint_sensor::get_device_settings() const noexcept
{
  return settings_;
}

result<device_settings> fingerprint_sensor::query_device_settings() noexcept
{
  spdlog::debug("querrying device settings...");
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
  spdlog::debug("setting baud rate...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::set_security_level_setting(security_level_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{static_cast<std::uint8_t>(device_setting_index::security_level_setting), static_cast<std::uint8_t>(setting)};
  spdlog::debug("setting security level...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::set_packet_length_setting(packet_length_setting setting) noexcept
{
  typename command_traits<command_code::write_system_parameter>::request req{static_cast<std::uint8_t>(device_setting_index::packet_length_setting), static_cast<std::uint8_t>(setting)};
  spdlog::debug("setting packet data length...");
  return executor_.execute<command_code::write_system_parameter>(req);
}

void_result fingerprint_sensor::soft_reset_device() noexcept
{
  spdlog::debug("performing soft reset...");
  return executor_.execute<command_code::soft_reset_device>({});
}

// --- led management

void_result fingerprint_sensor::set_led_setting(led_mode_setting mode, uint8_t speed, led_color_setting color, uint8_t cycles) noexcept
{
  spdlog::debug("setting led configuration");
  return executor_.execute<command_code::set_led_config>({static_cast<std::uint8_t>(mode), speed, static_cast<std::uint8_t>(color), cycles});
}

void_result fingerprint_sensor::turn_led_on() noexcept
{
  spdlog::debug("turning led on...");
  return executor_.execute<command_code::turn_led_on>({});
}

void_result fingerprint_sensor::turn_led_off() noexcept
{
  spdlog::debug("turning led off...");
  return executor_.execute<command_code::turn_led_off>({});
}

// --- security

void_result fingerprint_sensor::set_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept
{
  spdlog::debug("setting device password...");
  return executor_.execute<command_code::set_device_password>({std::move(password)});
}

void_result fingerprint_sensor::verify_device_password(carbio::scoped_zero<std::uint32_t> password) noexcept
{
  spdlog::debug("verifying device password...");
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
  spdlog::debug("capturing fingerprint image...");
  return executor_.execute<command_code::capture_image>({});
}

void_result fingerprint_sensor::extract_features(std::uint8_t buffer_id)
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  spdlog::debug("extracting feature points of the fingerprint image...");
  return executor_.execute<command_code::extract_features>(std::array<std::uint8_t, 1>{buffer_id});
}

void_result fingerprint_sensor::merge_model() noexcept
{
  spdlog::debug("merging feature points into template model...");
  return executor_.execute<command_code::merge_model>({});
}

void_result fingerprint_sensor::store_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  spdlog::debug("storing template model...");
  return executor_.execute<command_code::store_model>({buffer_id, page_id});
}

void_result fingerprint_sensor::load_model(std::uint16_t page_id, std::uint8_t buffer_id) noexcept
{
  spdlog::debug("loading template model...");
  return executor_.execute<command_code::load_model>({buffer_id, page_id});
}

void_result fingerprint_sensor::upload_model(std::span<const std::uint8_t> buffer, std::uint8_t buffer_id) noexcept
{

  auto result = executor_.execute<command_code::upload_model>({buffer_id});
  if (!result)
    return make_error(result.error());
  spdlog::debug("uploading template model...");
  return executor_.send_data_packets(buffer);
}

result<locked_buffer<std::uint8_t>> fingerprint_sensor::download_model(std::uint8_t buffer_id) noexcept
{
  // First send the download command
  spdlog::debug("downloading template model...");
  auto result = executor_.execute<command_code::download_model>({buffer_id});
  if (!result)
    return make_error(result.error());

  // Then receive data packets (returns locked_buffer)
  auto data_result = executor_.receive_data_packets();
  if (!data_result)
    return make_error(data_result.error());

  spdlog::debug("template model downloaded successfully ({} bytes)", data_result->size());
  return make_success(std::move(*data_result));
}

// --- database ops

void_result fingerprint_sensor::erase_model(std::uint16_t page_id, std::uint16_t count) noexcept
{
  spdlog::debug("erasing template model...");
  return executor_.execute<command_code::erase_model>({page_id, count});
}

void_result fingerprint_sensor::clear_database() noexcept
{
  spdlog::debug("clearing database...");
  return executor_.execute<command_code::clear_database>({});
}

result<match_query_info> fingerprint_sensor::match_model() noexcept
{
  spdlog::debug("matching template model...");
  return executor_.execute<command_code::match_model>({});
}

result<search_query_info> fingerprint_sensor::search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  spdlog::debug("searching template model...");
  return executor_.execute<command_code::search_model>({buffer_id, page_id, count});
}

result<search_query_info> fingerprint_sensor::fast_search_model(std::uint16_t page_id, std::uint8_t buffer_id, std::uint16_t count) noexcept
{
  spdlog::debug("fast searching template model...");
  return executor_.execute<command_code::fast_search_model>({buffer_id, page_id, count});
}

result<std::uint16_t> fingerprint_sensor::model_count() noexcept
{
  spdlog::debug("view template model count...");
  return executor_.execute<command_code::count_model>({});
}

result<std::vector<std::uint8_t>> fingerprint_sensor::read_index_table(std::span<std::uint8_t> data) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning

  spdlog::debug("reading index table...");
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

  spdlog::debug("writing notepad page {}...", page_number);
  return executor_.execute<command_code::write_notepad>(req);
}

result<std::array<std::uint8_t, 32>> fingerprint_sensor::read_notepad(std::uint8_t page_number) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  spdlog::debug("reading notepad page {}...", page_number);
  // Use explicit array construction to avoid stack protector warning
  std::array<std::uint8_t, 1> request{page_number};
  return executor_.execute<command_code::read_notepad>(request);
}

void_result fingerprint_sensor::enroll(std::uint16_t position, std::uint16_t sample_count)
{
  if (sample_count < 2)
  {
    spdlog::error("sample_count must be >= 2");
    return make_error(status_code::invalid_device_configuration);
  }
  if (sample_count < 8)
    spdlog::warn("Using fewer than recommended samples (recommended >= 8) may reduce reliability.");
  spdlog::info("Enrolling fingerprint with {} samples...", sample_count);
  if (auto r = capture_and_extract(1); !r) // sample 1
    return make_error(r.error());

  for (std::uint16_t s = 2; s <= sample_count; ++s) // sample 2 .. n
  {
    if (auto r = capture_and_extract(2); !r)
      return make_error(r.error());
    if (auto r = merge_model(); !r)
      return make_error(r.error());
  }
  if (auto r = store_model(position, 1); !r)
    return make_error(r.error());
  spdlog::info("Enrollment complete with {} samples", sample_count);
  return make_success();
}

void_result fingerprint_sensor::enroll_with_progress(std::uint16_t position, std::function<void(int, int, const char*)> callback, std::uint16_t sample_count)
{
  if (sample_count < 2)
  {
    spdlog::error("sample_count must be >= 2");
    return make_error(status_code::invalid_device_configuration);
  }
  if (sample_count < 8)
    spdlog::warn("Using fewer than recommended samples (recommended >= 8) may reduce reliability.");

  spdlog::info("Enrolling fingerprint with {} samples...", sample_count);

  // Sample 1
  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  callback(1, sample_count, "Sample 1 captured");

  // Samples 2 through sample_count
  for (std::uint16_t s = 2; s <= sample_count; ++s)
  {
    if (auto r = capture_and_extract(2); !r)
      return make_error(r.error());
    if (auto r = merge_model(); !r)
      return make_error(r.error());
    callback(s, sample_count, "Sample captured");
  }

  // Store the final model
  if (auto r = store_model(position, 1); !r)
    return make_error(r.error());

  spdlog::info("Enrollment complete with {} samples", sample_count);
  callback(sample_count, sample_count, "Enrollment complete");
  return make_success();
}


result<match_query_info> fingerprint_sensor::verify(std::uint16_t position)
{
  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  if (auto r = load_model(position, 2); !r)
    return make_error(r.error());
  auto r = match_model();
  if (!r)
    return make_error(r.error());
  return r;
}

result<search_query_info> fingerprint_sensor::identify()
{
  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  auto r = fast_search_model(0, 1, settings_.capacity);
  if (!r)
    return make_error(r.error());
  return r;
}

result<match_query_info> fingerprint_sensor::verify_with_progress(std::uint16_t position, std::function<void(int, const char*)> callback)
{
  callback(0, "Place finger on sensor");

  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  callback(33, "Fingerprint captured");

  if (auto r = load_model(position, 2); !r)
    return make_error(r.error());
  callback(66, "Template loaded");

  auto r = match_model();
  if (!r)
    return make_error(r.error());
  callback(100, "Verification complete");

  return r;
}

result<search_query_info> fingerprint_sensor::identify_with_progress(std::function<void(int, const char*)> callback)
{
  callback(0, "Place finger on sensor");

  if (auto r = capture_and_extract(1); !r)
    return make_error(r.error());
  callback(50, "Fingerprint captured");

  auto r = fast_search_model(0, 1, settings_.capacity);
  if (!r)
    return make_error(r.error());
  callback(100, "Search complete");

  return r;
}

void_result fingerprint_sensor::capture_and_extract(std::uint16_t position) noexcept
{
  spdlog::info("Place finger on sensor...");
  for (;;)
  {
    if (auto r = capture_image(); !r) // failed to capture
    {
      if (is_retryable(r.error()))
      {
        spdlog::trace("{}", message(r.error()));
        continue; // retry
      }
      spdlog::error("{}", message(r.error()));
      return make_error(r.error()); // fatal: non-recoverable
    }
    break; // done
  }

  spdlog::info("Extracting features...");
  for (;;)
  {
    if (auto r = extract_features(position); !r) // failed to extract
    {
      if (is_retryable(r.error()))
      {
        spdlog::trace("{}", message(r.error()));
        continue; // retry
      }
      spdlog::error("{}", message(r.error()));
      return make_error(r.error()); // fatal: non-recoverable
    }
    break; // done
  }
  // no errors
  spdlog::info("Remove finger...");
  for (;;)
  {
    if (auto r = capture_image(); !r)
    {
      if (status_code::no_finger == r.error())
      {
        spdlog::debug("Finger removed.");
        break;
      }
      if (!is_retryable(r.error()))
      { 
        spdlog::error("{}", message(r.error()));
        return make_error(r.error());
      }
    }
  }
  spdlog::info("Fingerprint captured.");
  return make_success();
}
} // namespace carbio
