#ifndef CARBIO_COMMAND_TRAITS_H
#define CARBIO_COMMAND_TRAITS_H

#include "carbio/command_code.h"

#include "carbio/device_setting_info.h"
#include "carbio/match_query_info.h"
#include "carbio/search_query_info.h"
#include "carbio/result.h"

#include <array>
#include <vector>

namespace carbio
{
// ----------------------
// command_traits
// ----------------------

template<command_code code>
struct command_traits;

// ----------------------
// Basic system control
// ----------------------

template<> struct command_traits<command_code::capture_image>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::extract_features>
{
  using request = std::array<std::uint8_t, 1>;
  using response = void_result;
};

template<> struct command_traits<command_code::create_model>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::store_model>
{
  struct request { std::uint8_t buffer_id; std::uint16_t page_id; };
  using response = void_result;
};

template<> struct command_traits<command_code::load_model>
{
  struct request { std::uint8_t buffer_id; std::uint16_t page_id; };
  using response = void_result;
};

template<> struct command_traits<command_code::upload_model>
{
  struct request { std::uint8_t buffer_id; };
  using response = result<std::array<std::uint8_t, 512>>;
};

template<> struct command_traits<command_code::download_model>
{
  struct request { std::uint8_t buffer_id; };
  using response = void_result;
};

// ----------------------
// Database operations
// ----------------------

template<> struct command_traits<command_code::upload_image>
{
  using request = std::tuple<>;
  using response = result<std::array<std::uint8_t, 512>>;
};

template<> struct command_traits<command_code::download_image>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::erase_model>
{
  struct request { std::uint16_t page_id; std::uint16_t count; };
  using response = void_result;
};

template<> struct command_traits<command_code::clear_database>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::match_model>
{
  using request = std::tuple<>;
  using response = result<match_query_info>;
};

template<> struct command_traits<command_code::search_model>
{
  struct request { std::uint8_t buffer_id; std::uint16_t page_id; std::uint16_t count;};
  using response = result<search_query_info>;
};

template<> struct command_traits<command_code::fast_search_model>
{
  struct request { std::uint8_t buffer_id; std::uint16_t page_id; std::uint16_t count;};
  using response = result<search_query_info>;
};

// ----------------------
// System & info queries
// ----------------------

template<> struct command_traits<command_code::count_model>
{
  using request  = std::tuple<>; 
  using response = result<std::uint16_t>;
};

template<> struct command_traits<command_code::read_system_parameter>
{
  using request  = std::tuple<>; 
  using response = result<device_setting_info>;
};

template<> struct command_traits<command_code::read_index_table>
{
  using request = std::array<std::uint8_t, 1>;
  using response = result<std::array<std::uint8_t, 32>>;
};

// ----------------------
// Security & password
// ----------------------

template<>
struct command_traits<command_code::set_device_password>
{
  struct request { std::uint32_t password; };
  using response = void_result;
};

template<>
struct command_traits<command_code::verify_device_password>
{
  struct request { std::uint32_t password; };
  using response = void_result;
};

// ----------------------
// Control functions
// ----------------------

template<> struct command_traits<command_code::write_system_parameter>
{
  struct request { std::uint8_t index; std::uint8_t value; };
  using response = void_result;
};

template<> struct command_traits<command_code::set_led_config>
{
  struct request { uint8_t mode; uint8_t speed; uint8_t color; uint8_t count; };
  using response = void_result;
};

template<> struct command_traits<command_code::turn_led_on>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::turn_led_off>
{
  using request = std::tuple<>;
  using response = void_result;
};

template<> struct command_traits<command_code::soft_reset_device>
{
  using request = std::tuple<>;
  using response = void_result;
};
} /* namespace carbio */

#endif // CARBIO_COMMAND_TRAITS_H