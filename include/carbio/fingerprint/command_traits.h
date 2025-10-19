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

#include "carbio/fingerprint/command_code.h"
#include "carbio/fingerprint/device_setting_info.h"
#include "carbio/fingerprint/match_query_info.h"
#include "carbio/fingerprint/result.h"
#include "carbio/fingerprint/search_query_info.h"
#include "carbio/utility/secure_value.h"

#include <array>
#include <vector>

namespace carbio::fingerprint {

template <command_code code> struct command_traits;

// --- system control ---

template <> struct command_traits<command_code::capture_image> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::extract_features> {
  using request = std::array<std::uint8_t, 1>;
  using response = void_result;
};

template <> struct command_traits<command_code::create_model> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::store_model> {
  struct request {
    std::uint8_t buffer_id;
    std::uint16_t page_id;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::load_model> {
  struct request {
    std::uint8_t buffer_id;
    std::uint16_t page_id;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::upload_model> {
  struct request {
    std::uint8_t buffer_id;
  };
  using response = result<std::array<std::uint8_t, 512>>;
};

template <> struct command_traits<command_code::download_model> {
  struct request {
    std::uint8_t buffer_id;
  };
  using response = void_result;
};

// --- database ops ---

template <> struct command_traits<command_code::upload_image> {
  using request = std::tuple<>;
  using response = result<std::array<std::uint8_t, 512>>;
};

template <> struct command_traits<command_code::download_image> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::erase_model> {
  struct request {
    std::uint16_t page_id;
    std::uint16_t count;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::clear_database> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::match_model> {
  using request = std::tuple<>;
  using response = result<match_query_info>;
};

template <> struct command_traits<command_code::search_model> {
  struct request {
    std::uint8_t buffer_id;
    std::uint16_t page_id;
    std::uint16_t count;
  };
  using response = result<search_query_info>;
};

template <> struct command_traits<command_code::fast_search_model> {
  struct request {
    std::uint8_t buffer_id;
    std::uint16_t page_id;
    std::uint16_t count;
  };
  using response = result<search_query_info>;
};

// --- system queries ---

template <> struct command_traits<command_code::count_model> {
  using request = std::tuple<>;
  using response = result<std::uint16_t>;
};

template <> struct command_traits<command_code::read_system_parameter> {
  using request = std::tuple<>;
  using response = result<device_setting_info>;
};

template <> struct command_traits<command_code::read_index_table> {
  using request = std::array<std::uint8_t, 1>;
  using response = result<std::array<std::uint8_t, 32>>;
};

// --- security & password mgmt. ---

template <> struct command_traits<command_code::set_device_password> {
  struct request {
    carbio::secure_value<std::uint32_t> password;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::verify_device_password> {
  struct request {
    carbio::secure_value<std::uint32_t> password;
  };
  using response = void_result;
};

// --- device control ---

template <> struct command_traits<command_code::write_system_parameter> {
  struct request {
    std::uint8_t index;
    std::uint8_t value;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::set_led_config> {
  struct request {
    uint8_t mode;
    uint8_t speed;
    uint8_t color;
    uint8_t count;
  };
  using response = void_result;
};

template <> struct command_traits<command_code::turn_led_on> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::turn_led_off> {
  using request = std::tuple<>;
  using response = void_result;
};

template <> struct command_traits<command_code::soft_reset_device> {
  using request = std::tuple<>;
  using response = void_result;
};
} /* namespace carbio::fingerprint */
