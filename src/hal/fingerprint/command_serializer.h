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

#include "fingerprint/command_code.h"
#include "fingerprint/command_traits.h"
#include "utility/endian.h"

#include <algorithm>

namespace carbio
{
template <command_code code>
std::vector<std::uint8_t> serialize_request(typename command_traits<code>::request const& req) noexcept;

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::capture_image>(typename command_traits<command_code::capture_image>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::merge_model>(typename command_traits<command_code::merge_model>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::upload_image>(typename command_traits<command_code::upload_image>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::clear_database>(typename command_traits<command_code::clear_database>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::match_model>(typename command_traits<command_code::match_model>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::count_model>(typename command_traits<command_code::count_model>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::read_system_parameter>(typename command_traits<command_code::read_system_parameter>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::turn_led_on>(typename command_traits<command_code::turn_led_on>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::turn_led_off>(typename command_traits<command_code::turn_led_off>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::soft_reset_device>(typename command_traits<command_code::soft_reset_device>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::extract_features>(typename command_traits<command_code::extract_features>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req[0]};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::read_index_table>(typename command_traits<command_code::read_index_table>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req[0]};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::store_model>(typename command_traits<command_code::store_model>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(3);
  data[0] = req.buffer_id;
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(1, 2), req.page_id);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::load_model>(typename command_traits<command_code::load_model>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(3);
  data[0] = req.buffer_id;
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(1, 2), req.page_id);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::upload_model>(typename command_traits<command_code::upload_model>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req.buffer_id};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::download_model>(typename command_traits<command_code::download_model>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req.buffer_id};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::download_image>(typename command_traits<command_code::download_image>::request const&) noexcept
{
  return {};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::erase_model>(typename command_traits<command_code::erase_model>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(4);
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(0, 2), req.page_id);
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(2, 2), req.count);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::search_model>(typename command_traits<command_code::search_model>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(5);
  data[0] = req.buffer_id;
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(1, 2), req.page_id);
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(3, 2), req.count);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::fast_search_model>(typename command_traits<command_code::fast_search_model>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(5);
  data[0] = req.buffer_id;
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(1, 2), req.page_id);
  carbio::write_be<std::uint16_t>(std::span{data}.subspan(3, 2), req.count);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::set_device_password>(typename command_traits<command_code::set_device_password>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(4);
  carbio::write_be<std::uint32_t>(std::span{data}, req.password.get());
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::verify_device_password>(typename command_traits<command_code::verify_device_password>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(4);
  carbio::write_be<std::uint32_t>(std::span{data}, req.password.get());
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::write_system_parameter>(typename command_traits<command_code::write_system_parameter>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req.index, req.value};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::set_led_config>(typename command_traits<command_code::set_led_config>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req.mode, req.speed, req.color, req.count};
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::write_notepad>(typename command_traits<command_code::write_notepad>::request const& req) noexcept
{
  std::vector<std::uint8_t> data(33);
  data[0] = req.page_number;
  std::copy(req.data.begin(), req.data.end(), data.begin() + 1);
  return data;
}

template <>
inline std::vector<std::uint8_t> serialize_request<command_code::read_notepad>(typename command_traits<command_code::read_notepad>::request const& req) noexcept
{
  // Stack protector guard: ensure we have a local array >= 8 bytes
  volatile std::uint8_t stack_guard[8] = {0};
  (void)stack_guard; // Prevent unused variable warning
  return {req[0]};
}
} // namespace carbio
