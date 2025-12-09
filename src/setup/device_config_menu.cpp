#include "device_config_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>

void device_config_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nsystem config:\n");
    fmt::print("1) set baud rate\n");
    fmt::print("2) set security level\n");
    fmt::print("3) set packet size\n");
    fmt::print("4) set device password\n");
    fmt::print("5) show current settings\n");
    fmt::print("6) soft reset device\n");
    fmt::print("x) back\n");
    fmt::print("----------\n");
    fmt::print("> ");
    std::getline(std::cin, input);
    const auto cmd = input.front();
    if (cmd == 'X' || cmd == 'x')
    {
      return;
    }
    switch (cmd)
    {
    case '1':
      baud_rate_menu(s);
      break;
    case '2':
      security_level_menu(s);
      break;
    case '3':
      packet_length_menu(s);
      break;
    case '4':
      device_password_menu(s);
      break;
    case '5':
      current_settings_menu(s);
      break;
    case '6':
      soft_reset_menu(s);
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
    }
  }
}

namespace
{
constexpr auto serial_port = "/dev/ttyAMA0";
}

void baud_rate_menu(carbio::fingerprint_sensor& s) noexcept
{
  struct baud_option
  {
    char key;
    const char* label;
    carbio::baud_rate_setting setting;
  };

  constexpr std::array<baud_option, 12> kOptions{{
      {'1', "9600", carbio::baud_rate_setting::_9600},
      {'2', "19200", carbio::baud_rate_setting::_19200},
      {'3', "28800", carbio::baud_rate_setting::_28800},
      {'4', "38400", carbio::baud_rate_setting::_38400},
      {'5', "48000", carbio::baud_rate_setting::_48000},
      {'6', "57600", carbio::baud_rate_setting::_57600},
      {'7', "67200", carbio::baud_rate_setting::_67200},
      {'8', "76800", carbio::baud_rate_setting::_76800},
      {'9', "86400", carbio::baud_rate_setting::_86400},
      {'a', "96000", carbio::baud_rate_setting::_96000},
      {'b', "105600", carbio::baud_rate_setting::_105600},
      {'c', "115200", carbio::baud_rate_setting::_115200},
  }};

  auto reconnect_sensor = [&]() {
    s.disconnect();
    s.connect(serial_port);
  };

  std::string input;
  for (;;)
  {
    fmt::print("----------\n\nset baud rate:\n");
    for (const auto& option : kOptions)
    {
      fmt::print("{} ) {}\n", option.key, option.label);
    }
    fmt::print("x) back\n----------\n> ");

    if (!std::getline(std::cin, input) || input.empty())
      continue;

    const auto raw_cmd = input.front();
    const auto cmd = static_cast<char>(std::tolower(static_cast<unsigned char>(raw_cmd)));

    if (cmd == 'x')
      return;

    const auto selection = std::find_if(kOptions.begin(), kOptions.end(),
                                        [cmd](const auto& option) { return option.key == cmd; });

    if (selection == kOptions.end())
    {
      fmt::print("unknown command '{}'\n", raw_cmd);
      continue;
    }

    if (auto r = s.set_baud_rate_setting(selection->setting); !r)
    {
      fmt::print("{}", message(r.error()));
      continue;
    }

    reconnect_sensor();
  }
}

void security_level_menu(carbio::fingerprint_sensor& s) noexcept
{
  struct option
  {
    char key;
    const char* label;
    carbio::security_level_setting value;
  };

  constexpr std::array<option, 5> kOptions{{
      {'1', "lowest", carbio::security_level_setting::lowest},
      {'2', "low", carbio::security_level_setting::low},
      {'3', "balanced", carbio::security_level_setting::balanced},
      {'4', "high", carbio::security_level_setting::high},
      {'5', "highest", carbio::security_level_setting::highest},
  }};

  std::string input;
  for (;;)
  {
    fmt::print("----------\n\nset security level:\n");
    for (const auto& option : kOptions)
    {
      fmt::print("{} ) {}\n", option.key, option.label);
    }
    fmt::print("x) back\n----------\n> ");

    if (!std::getline(std::cin, input) || input.empty())
      continue;

    const auto raw_cmd = input.front();
    const auto cmd = static_cast<char>(std::tolower(static_cast<unsigned char>(raw_cmd)));

    if (cmd == 'x')
      return;

    const auto selection = std::find_if(kOptions.begin(), kOptions.end(),
                                        [cmd](const auto& option) { return option.key == cmd; });
    if (selection == kOptions.end())
    {
      fmt::print("unknown command '{}'\n", raw_cmd);
      continue;
    }

    if (auto r = s.set_security_level_setting(selection->value); !r)
    {
      fmt::print("{}", message(r.error()));
    }
  }
}

void packet_length_menu(carbio::fingerprint_sensor& s) noexcept
{
  struct option
  {
    char key;
    const char* label;
    carbio::packet_length_setting value;
  };

  constexpr std::array<option, 4> kOptions{{
      {'1', "32", carbio::packet_length_setting::_32},
      {'2', "64", carbio::packet_length_setting::_64},
      {'3', "128", carbio::packet_length_setting::_128},
      {'4', "256", carbio::packet_length_setting::_256},
  }};

  std::string input;
  for (;;)
  {
    fmt::print("----------\n\nset packet length:\n");
    for (const auto& option : kOptions)
    {
      fmt::print("{} ) {}\n", option.key, option.label);
    }
    fmt::print("x) back\n----------\n> ");

    if (!std::getline(std::cin, input) || input.empty())
      continue;

    const auto raw_cmd = input.front();
    const auto cmd = static_cast<char>(std::tolower(static_cast<unsigned char>(raw_cmd)));

    if (cmd == 'x')
      return;

    const auto selection = std::find_if(kOptions.begin(), kOptions.end(),
                                        [cmd](const auto& option) { return option.key == cmd; });
    if (selection == kOptions.end())
    {
      fmt::print("unknown command '{}'\n", raw_cmd);
      continue;
    }

    if (auto r = s.set_packet_length_setting(selection->value); !r)
    {
      fmt::print("{}", message(r.error()));
    }
  }
}

void device_password_menu(carbio::fingerprint_sensor& s) noexcept {}

void current_settings_menu(carbio::fingerprint_sensor& s) noexcept
{
  fmt::print("----------\n");
  fmt::print("\ncurrent device settings:\n");
  if (auto r = s.update_device_settings(); !r)
  {
    fmt::print("{}", message(r.error()));
    return;
  }
  fmt::print("{}\n", to_json(s.get_device_settings()));
  fmt::print("----------\n");
}

void soft_reset_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nsoft reset device:\n");
    fmt::print("y) yes\n");
    fmt::print("n) no\n");
    fmt::print("----------\n");
    fmt::print("> ");
    std::getline(std::cin, input);
    const auto cmd = input.front();
    if (cmd == 'N' || cmd == 'n')
    {
      return;
    }
    switch (cmd)
    {
    case 'y':
    case 'Y':
      if (auto r = s.soft_reset_device(); !r)
      {
        fmt::print("error: {}\n", message(r.error()));
        return;
      }
      s.disconnect();
      s.connect(serial_port);
      fmt::print("{}\n", to_json(s.get_device_settings()));
      fmt::print("device has been soft reset\n");
      return;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      return;
    }
  }
}