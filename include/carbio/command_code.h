/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
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

#ifndef CARBIO_COMMAND_CODE_H
#define CARBIO_COMMAND_CODE_H

#include <cstdint>
#include <string_view>

namespace carbio
{
/*!
 * @brief The command code.
 */
enum class command_code : std::uint8_t
{
  capture_image          = 0x01, /*!< capture fingerprint image */
  extract_features       = 0x02, /*!< extract fingerprint features */
  match_model            = 0x03, /*!< match fingerprint */
  search_model           = 0x04, /*!< search fingerprint */
  create_model           = 0x05, /*!< create template model */
  store_model            = 0x06, /*!< Store template model */
  load_model             = 0x07, /*!< Load template model */
  upload_model           = 0x08, /*!< Upload template model */
  download_model         = 0x09, /*!< Download template model */
  upload_image           = 0x0A, /*!< Upload image */
  download_image         = 0x0B, /*!< Download image */
  erase_model            = 0x0C, /*!< Erase template model */
  clear_database         = 0x0D, /*!< Erase database */
  write_system_parameter = 0x0E, /*!< Write system parameters */
  read_system_parameter  = 0x0F, /*!< Read system parameters */
  set_device_password    = 0x12, /*!< Set device password */
  verify_device_password = 0x13, /*!< Verify device password */
  fast_search_model      = 0x1B, /*!< fast search fingerprint */
  count_model            = 0x1D, /*!< Get template count */
  read_index_table       = 0x1F, /*!< read index table */
  set_led_config         = 0x35, /*!< set led config */
  soft_reset_device      = 0x3D, /*!< soft reset device */
  turn_led_on            = 0x50, /*!< turn led on */
  turn_led_off           = 0x51  /*!< turn led off */
};

/*!
 * @brief Convert command code to string representation.
 * @param code The command code.
 * @returns The string representation of command.
 */
[[nodiscard]] inline constexpr std::string_view get_name(command_code code)
{
  using namespace std::literals;
  switch (code)
  {
    case command_code::capture_image:
      return "capture_image"sv;
    case command_code::extract_features:
      return "extract_features"sv;
    case command_code::match_model:
      return "match_model"sv;
    case command_code::search_model:
      return "search_model"sv;
    case command_code::create_model:
      return "create_model"sv;
    case command_code::store_model:
      return "store_model"sv;
    case command_code::load_model:
      return "load_model"sv;
    case command_code::upload_model:
      return "upload_model"sv;
    case command_code::download_model:
      return "download_model"sv;
    case command_code::upload_image:
      return "upload_image"sv;
    case command_code::download_image:
      return "download_image"sv;
    case command_code::erase_model:
      return "erase_model"sv;
    case command_code::clear_database:
      return "clear_database"sv;
    case command_code::write_system_parameter:
      return "write_system_parameter"sv;
    case command_code::read_system_parameter:
      return "read_system_parameter"sv;
    case command_code::set_device_password:
      return "set_device_password"sv;
    case command_code::verify_device_password:
      return "verify_device_password"sv;
    case command_code::fast_search_model:
      return "fast_search_model"sv;
    case command_code::count_model:
      return "count_model"sv;
    case command_code::read_index_table:
      return "read_index_table"sv;
    case command_code::set_led_config:
      return "set_led_config"sv;
    case command_code::soft_reset_device:
      return "soft_reset_device"sv;
    case command_code::turn_led_on:
      return "turn_led_on"sv;
    case command_code::turn_led_off:
      return "turn_led_off"sv;
    default:
      return "unknown_command"sv;
  }
}

/*!
 * @brief Convert the command code to hexadecimal representation.
 * @param code The command code.
 * @returns The hexadecimal representation of command.
 */
[[nodiscard]] inline constexpr std::string_view get_hex(command_code code)
{
  using namespace std::literals;
  switch (code)
  {
    case command_code::capture_image:
      return "0x01"sv;
    case command_code::extract_features:
      return "0x02"sv;
    case command_code::match_model:
      return "0x03"sv;
    case command_code::search_model:
      return "0x04"sv;
    case command_code::create_model:
      return "0x05"sv;
    case command_code::store_model:
      return "0x06"sv;
    case command_code::load_model:
      return "0x07"sv;
    case command_code::upload_model:
      return "0x08"sv;
    case command_code::download_model:
      return "0x09"sv;
    case command_code::upload_image:
      return "0x0A"sv;
    case command_code::download_image:
      return "0x0B"sv;
    case command_code::erase_model:
      return "0x0C"sv;
    case command_code::clear_database:
      return "0x0D"sv;
    case command_code::write_system_parameter:
      return "0x0E"sv;
    case command_code::read_system_parameter:
      return "0x0F"sv;
    case command_code::set_device_password:
      return "0x12"sv;
    case command_code::verify_device_password:
      return "0x13"sv;
    case command_code::fast_search_model:
      return "0x1B"sv;
    case command_code::count_model:
      return "0x1D"sv;
    case command_code::read_index_table:
      return "0x1F"sv;
    case command_code::set_led_config:
      return "0x35"sv;
    case command_code::soft_reset_device:
      return "0x3D"sv;
    case command_code::turn_led_on:
      return "0x50"sv;
    case command_code::turn_led_off:
      return "0x51"sv;
    default:
      return "0x00"sv;
  }
}

/*!
 * @brief Convert the command code to human readable message.
 * @param code The command code.
 * @returns The human readable string that describes the command.
 */
[[nodiscard]] inline constexpr std::string_view get_message(command_code code)
{
  using namespace std::literals;
  switch (code)
  {
    case command_code::capture_image:
      return "capture fingerprint image"sv;
    case command_code::extract_features:
      return "extract fingerprint features"sv;
    case command_code::match_model:
      return "match fingerprint"sv;
    case command_code::search_model:
      return "search fingerprint"sv;
    case command_code::create_model:
      return "create template model"sv;
    case command_code::store_model:
      return "store template model"sv;
    case command_code::load_model:
      return "load template model"sv;
    case command_code::upload_model:
      return "upload template model"sv;
    case command_code::download_model:
      return "download template model"sv;
    case command_code::upload_image:
      return "upload image"sv;
    case command_code::download_image:
      return "download image"sv;
    case command_code::erase_model:
      return "erase template model"sv;
    case command_code::clear_database:
      return "erase database contents"sv;
    case command_code::write_system_parameter:
      return "write system parameter"sv;
    case command_code::read_system_parameter:
      return "read system parameter"sv;
    case command_code::set_device_password:
      return "set device password"sv;
    case command_code::verify_device_password:
      return "verify device password"sv;
    case command_code::fast_search_model:
      return "fast search model"sv;
    case command_code::count_model:
      return "count template models"sv;
    case command_code::read_index_table:
      return "read index table"sv;
    case command_code::set_led_config:
      return "set led setting"sv;
    case command_code::soft_reset_device:
      return "soft reset device"sv;
    case command_code::turn_led_on:
      return "turn led on"sv;
    case command_code::turn_led_off:
      return "turn led off"sv;
    default:
      return "unknown command"sv;
  }
}

} // namespace carbio

#endif // CARBIO_COMMAND_CODE_H
