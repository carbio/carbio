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
typename command_traits<code>::response deserialize_response(std::span<const std::uint8_t> data) noexcept;

template <>
inline void_result deserialize_response<command_code::capture_image>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::extract_features>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::merge_model>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::store_model>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::load_model>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::download_model>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::download_image>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::erase_model>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::clear_database>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::set_device_password>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::verify_device_password>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::write_system_parameter>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::set_led_config>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::soft_reset_device>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::turn_led_on>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline void_result deserialize_response<command_code::turn_led_off>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline result<std::uint16_t> deserialize_response<command_code::count_model>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 2)
    return make_error(status_code::bad_packet);
  return make_success(carbio::read_be<std::uint16_t>(data));
}

template <>
inline result<std::array<std::uint8_t, 512>> deserialize_response<command_code::upload_model>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 512)
    return make_error(status_code::bad_packet);

  std::array<std::uint8_t, 512> result;
  std::copy_n(data.begin(), 512, result.begin());
  return make_success(std::move(result));
}

template <>
inline result<std::array<std::uint8_t, 512>> deserialize_response<command_code::upload_image>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 512)
    return make_error(status_code::bad_packet);

  std::array<std::uint8_t, 512> result;
  std::copy_n(data.begin(), 512, result.begin());
  return make_success(std::move(result));
}

template <>
inline result<std::array<std::uint8_t, 32>> deserialize_response<command_code::read_index_table>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 32)
    return make_error(status_code::bad_packet);

  std::array<std::uint8_t, 32> result;
  std::copy_n(data.begin(), 32, result.begin());
  return make_success(std::move(result));
}

template <>
inline result<match_query_info> deserialize_response<command_code::match_model>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 2)
    return make_error(status_code::bad_packet);

  match_query_info info;
  info.confidence = carbio::read_be<std::uint16_t>(data);
  return make_success(std::move(info));
}

template <>
inline result<search_query_info> deserialize_response<command_code::search_model>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 4)
    return make_error(status_code::bad_packet);

  search_query_info info;
  info.index = carbio::read_be<std::uint16_t>(data.subspan(0, 2));
  info.confidence = carbio::read_be<std::uint16_t>(data.subspan(2, 2));
  return make_success(std::move(info));
}

template <>
inline result<search_query_info> deserialize_response<command_code::fast_search_model>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 4)
    return make_error(status_code::bad_packet);

  search_query_info info;
  info.index = carbio::read_be<std::uint16_t>(data.subspan(0, 2));
  info.confidence = carbio::read_be<std::uint16_t>(data.subspan(2, 2));

  return make_success(std::move(info));
}

template <>
inline result<device_settings> deserialize_response<command_code::read_system_parameter>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 16)
    return make_error(status_code::bad_packet);

  device_settings settings;
  settings.status = carbio::read_be<std::uint16_t>(data.subspan(0, 2));
  settings.id = carbio::read_be<std::uint16_t>(data.subspan(2, 2));
  settings.capacity = carbio::read_be<std::uint16_t>(data.subspan(4, 2));
  settings.security_level = carbio::read_be<std::uint16_t>(data.subspan(6, 2));
  settings.address = carbio::read_be<std::uint32_t>(data.subspan(8, 4));
  settings.length = carbio::read_be<std::uint16_t>(data.subspan(12, 2));
  settings.baudrate = carbio::read_be<std::uint16_t>(data.subspan(14, 2));

  return make_success(settings);
}

template <>
inline void_result deserialize_response<command_code::write_notepad>(std::span<const std::uint8_t>) noexcept
{
  return make_success();
}

template <>
inline result<std::array<std::uint8_t, 32>> deserialize_response<command_code::read_notepad>(std::span<const std::uint8_t> data) noexcept
{
  if (data.size() < 32)
    return make_error(status_code::bad_packet);

  std::array<std::uint8_t, 32> notepad_data;
  std::copy_n(data.begin(), 32, notepad_data.begin());
  return make_success(std::move(notepad_data));
}
} // namespace carbio