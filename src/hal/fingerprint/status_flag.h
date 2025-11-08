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

#include "fingerprint/status_code.h"

namespace carbio
{
enum class status_flag : std::uint8_t
{
  none = 0,
  transient = 1,      /*!< short-lived, worth retrying */
  user = 2,           /*!< user-related error */
  hardware = 4,       /*!< hardware-related error */
  database = 8,       /*!< database-related error */
  communication = 16, /*!< communication/network-related issues*/
  capture = 32,       /*!< capture/quality-related error */
  security = 64,      /*!< permission/authorization error */
  fatal = 128         /*!< non-recoverable error */
};

inline constexpr status_flag operator|(status_flag a, status_flag b) noexcept
{
  return static_cast<status_flag>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline constexpr status_flag operator&(status_flag a, status_flag b) noexcept
{
  return static_cast<status_flag>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

inline constexpr bool has_flag(status_flag a, status_flag b) noexcept
{
  return (static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b)) != 0;
}

inline constexpr status_flag flag_for(status_code s)
{
  using sc = status_code;
  using sf = status_flag;
  switch (s)
  {
  case sc::success:
    return sf::none;
  case sc::frame_error:
    return sf::transient | sf::communication;
  case sc::timeout:
    return sf::transient | sf::communication;
  case sc::communication_error:
    return sf::transient | sf::communication;
  case sc::bad_packet:
    return sf::transient | sf::communication;
  case sc::no_finger:
    return sf::user | sf::capture;
  case sc::no_frame:
    return sf::user | sf::capture;
  case sc::image_too_faint:
  case sc::image_too_blurry:
  case sc::image_too_distorted:
  case sc::image_too_few_features:
  case sc::image_capture_error:
  case sc::invalid_image_format:
    return sf::capture | sf::transient;
  case sc::no_match:
  case sc::not_found:
    return sf::user;
  case sc::enrollment_mismatch:
    return sf::user;
  case sc::index_out_of_range:
    return sf::fatal;
  case sc::device_authorization_required:
  case sc::permission_denied:
    return sf::security | sf::user;
  case sc::database_access_error:
  case sc::database_full:
  case sc::erase_failed:
  case sc::database_clear_failed:
    return sf::database;
  case sc::cannot_enter_low_power_mode:
    return sf::hardware | sf::transient;
  case sc::hardware_fault:
  case sc::flash_write_error:
  case sc::illegal_device_register:
  case sc::illegal_device_address:
  case sc::invalid_device_configuration:
  case sc::unsupported_command:
    return sf::hardware | sf::fatal;
  case sc::feature_upload_failed:
  case sc::image_upload_failed:
    return sf::transient;
  case sc::unknown_error:
    return sf::fatal;
  default:
    return sf::none;
  }
}

inline constexpr bool has_flag(status_code sc, status_flag sf)
{
  return has_flag(flag_for(sc), sf);
}

inline constexpr bool is_success(status_code s) noexcept
{
  return has_flag(s, status_flag::none);
}

inline constexpr bool is_transient_error(status_code s) noexcept
{
  return has_flag(s, status_flag::transient);
}

inline constexpr bool is_user_error(status_code s) noexcept
{
  return has_flag(s, status_flag::user);
}

inline constexpr bool is_hardware_error(status_code s) noexcept
{
  return has_flag(s, status_flag::hardware);
}

inline constexpr bool is_database_error(status_code s) noexcept
{
  return has_flag(s, status_flag::database);
}

inline constexpr bool is_communication_error(status_code s) noexcept
{
  return has_flag(s, status_flag::communication);
}

inline constexpr bool is_capture_error(status_code s) noexcept
{
  return has_flag(s, status_flag::capture);
}

inline constexpr bool is_security_error(status_code s) noexcept
{
  return has_flag(s, status_flag::security);
}

inline constexpr bool is_fatal_error(status_code s) noexcept
{
  return has_flag(s, status_flag::fatal);
}

inline constexpr bool is_retryable(status_code s) noexcept
{
  auto flags = flag_for(s);

  // Fatal or security errors are never retriable
  if (has_flag(flags, status_flag::fatal))
    return false;
  if (has_flag(flags, status_flag::security))
    return false;

  // Transient or capture errors are retriable (user can try again)
  return has_flag(flags, status_flag::transient) || has_flag(flags, status_flag::capture);
}
} // namespace carbio