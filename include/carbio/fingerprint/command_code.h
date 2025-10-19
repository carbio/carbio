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

#include <cstdint>

namespace carbio::fingerprint {
/*!
 * @brief The command code.
 */
enum class command_code : std::uint8_t {
  capture_image = 0x01,          /*!< capture fingerprint image */
  extract_features = 0x02,       /*!< extract fingerprint features */
  match_model = 0x03,            /*!< match fingerprint */
  search_model = 0x04,           /*!< search fingerprint */
  create_model = 0x05,           /*!< create template model */
  store_model = 0x06,            /*!< Store template model */
  load_model = 0x07,             /*!< Load template model */
  upload_model = 0x08,           /*!< Upload template model */
  download_model = 0x09,         /*!< Download template model */
  upload_image = 0x0A,           /*!< Upload image */
  download_image = 0x0B,         /*!< Download image */
  erase_model = 0x0C,            /*!< Erase template model */
  clear_database = 0x0D,         /*!< Erase database */
  write_system_parameter = 0x0E, /*!< Write system parameters */
  read_system_parameter = 0x0F,  /*!< Read system parameters */
  set_device_password = 0x12,    /*!< Set device password */
  verify_device_password = 0x13, /*!< Verify device password */
  fast_search_model = 0x1B,      /*!< fast search fingerprint */
  count_model = 0x1D,            /*!< Get template count */
  read_index_table = 0x1F,       /*!< read index table */
  set_led_config = 0x35,         /*!< set led config */
  soft_reset_device = 0x3D,      /*!< soft reset device */
  turn_led_on = 0x50,            /*!< turn led on */
  turn_led_off = 0x51            /*!< turn led off */
};

/*!
 * @brief Convert command code to string representation.
 * @param code The command code.
 * @returns The string representation of command.
 */
[[nodiscard]] inline constexpr const char *name(command_code code) {
  switch (code) {
  case command_code::capture_image:
    return "command_code::capture_image";
  case command_code::extract_features:
    return "command_code::extract_features";
  case command_code::match_model:
    return "command_code::match_model";
  case command_code::search_model:
    return "command_code::search_model";
  case command_code::create_model:
    return "command_code::create_model";
  case command_code::store_model:
    return "command_code::store_model";
  case command_code::load_model:
    return "command_code::load_model";
  case command_code::upload_model:
    return "command_code::upload_model";
  case command_code::download_model:
    return "command_code::download_model";
  case command_code::upload_image:
    return "command_code::upload_image";
  case command_code::download_image:
    return "command_code::download_image";
  case command_code::erase_model:
    return "command_code::erase_model";
  case command_code::clear_database:
    return "command_code::clear_database";
  case command_code::write_system_parameter:
    return "command_code::write_system_parameter";
  case command_code::read_system_parameter:
    return "command_code::read_system_parameter";
  case command_code::set_device_password:
    return "command_code::set_device_password";
  case command_code::verify_device_password:
    return "command_code::verify_device_password";
  case command_code::fast_search_model:
    return "command_code::fast_search_model";
  case command_code::count_model:
    return "command_code::count_model";
  case command_code::read_index_table:
    return "command_code::read_index_table";
  case command_code::set_led_config:
    return "command_code::set_led_config";
  case command_code::soft_reset_device:
    return "command_code::soft_reset_device";
  case command_code::turn_led_on:
    return "command_code::turn_led_on";
  case command_code::turn_led_off:
    return "command_code::turn_led_off";
  default:
    return "<unspecified enum value>";
  }
}

/*!
 * @brief Convert the command code to hexadecimal representation.
 * @param code The command code.
 * @returns The hexadecimal representation of command.
 */
[[nodiscard]] inline constexpr const char *hex_string(command_code code) {
  switch (code) {
  case command_code::capture_image:
    return "0x01";
  case command_code::extract_features:
    return "0x02";
  case command_code::match_model:
    return "0x03";
  case command_code::search_model:
    return "0x04";
  case command_code::create_model:
    return "0x05";
  case command_code::store_model:
    return "0x06";
  case command_code::load_model:
    return "0x07";
  case command_code::upload_model:
    return "0x08";
  case command_code::download_model:
    return "0x09";
  case command_code::upload_image:
    return "0x0A";
  case command_code::download_image:
    return "0x0B";
  case command_code::erase_model:
    return "0x0C";
  case command_code::clear_database:
    return "0x0D";
  case command_code::write_system_parameter:
    return "0x0E";
  case command_code::read_system_parameter:
    return "0x0F";
  case command_code::set_device_password:
    return "0x12";
  case command_code::verify_device_password:
    return "0x13";
  case command_code::fast_search_model:
    return "0x1B";
  case command_code::count_model:
    return "0x1D";
  case command_code::read_index_table:
    return "0x1F";
  case command_code::set_led_config:
    return "0x35";
  case command_code::soft_reset_device:
    return "0x3D";
  case command_code::turn_led_on:
    return "0x50";
  case command_code::turn_led_off:
    return "0x51";
  default:
    return "0xFF";
  }
}

/*!
 * @brief Convert the command code to human readable message.
 * @param code The command code.
 * @returns The human readable string that describes the command.
 */
[[nodiscard]] inline constexpr const char *message(command_code code) {
  switch (code) {
  case command_code::capture_image:
    return "capture fingerprint image";
  case command_code::extract_features:
    return "extract fingerprint features";
  case command_code::match_model:
    return "match fingerprint";
  case command_code::search_model:
    return "search fingerprint";
  case command_code::create_model:
    return "create template model";
  case command_code::store_model:
    return "store template model";
  case command_code::load_model:
    return "load template model";
  case command_code::upload_model:
    return "upload template model";
  case command_code::download_model:
    return "download template model";
  case command_code::upload_image:
    return "upload image";
  case command_code::download_image:
    return "download image";
  case command_code::erase_model:
    return "erase template model";
  case command_code::clear_database:
    return "erase database contents";
  case command_code::write_system_parameter:
    return "write system parameter";
  case command_code::read_system_parameter:
    return "read system parameter";
  case command_code::set_device_password:
    return "set device password";
  case command_code::verify_device_password:
    return "verify device password";
  case command_code::fast_search_model:
    return "fast search model";
  case command_code::count_model:
    return "count template models";
  case command_code::read_index_table:
    return "read index table";
  case command_code::set_led_config:
    return "set led setting";
  case command_code::soft_reset_device:
    return "soft reset device";
  case command_code::turn_led_on:
    return "turn led on";
  case command_code::turn_led_off:
    return "turn led off";
  default:
    return "<unspecified enum value>";
  }
}
} // namespace carbio::fingerprint
