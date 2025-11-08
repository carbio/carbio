#include "device_config_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include <fmt/core.h>

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

void baud_rate_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nset baud rate:\n");
    fmt::print("1) 9600\n");
    fmt::print("2) 19200\n");
    fmt::print("3) 28800\n");
    fmt::print("4) 38400\n");
    fmt::print("5) 48000\n");
    fmt::print("6) 57600\n");
    fmt::print("7) 67200\n");
    fmt::print("8) 76800\n");
    fmt::print("9) 86400\n");
    fmt::print("a) 96000\n");
    fmt::print("b) 105600\n");
    fmt::print("c) 115200\n");
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
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_9600); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '2':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_19200); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '3':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_28800); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '4':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_38400); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '5':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_48000); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '6':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_57600); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '7':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_67200); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '8':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_76800); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case '9':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_86400); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case 'A':
    case 'a':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_96000); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case 'B':
    case 'b':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_105600); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    case 'C':
    case 'c':
      if (auto r = s.set_baud_rate_setting(carbio::baud_rate_setting::_115200); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      s.disconnect();
      s.connect("/dev/ttyAMA0");
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
    }
  }
}

void security_level_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nset security level:\n");
    fmt::print("1) lowest\n");
    fmt::print("2) low\n");
    fmt::print("3) balanced\n");
    fmt::print("4) high\n");
    fmt::print("5) highest\n");
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
      if (auto r = s.set_security_level_setting(carbio::security_level_setting::lowest); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '2':
      if (auto r = s.set_security_level_setting(carbio::security_level_setting::low); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '3':
      if (auto r = s.set_security_level_setting(carbio::security_level_setting::balanced); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '4':
      if (auto r = s.set_security_level_setting(carbio::security_level_setting::high); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '5':
      if (auto r = s.set_security_level_setting(carbio::security_level_setting::highest); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
    }
  }
}

void packet_length_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nset packet length:\n");
    fmt::print("1) 32\n");
    fmt::print("2) 64\n");
    fmt::print("3) 128\n");
    fmt::print("4) 256\n");
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
      if (auto r = s.set_packet_length_setting(carbio::packet_length_setting::_32); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '2':
      if (auto r = s.set_packet_length_setting(carbio::packet_length_setting::_64); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '3':
      if (auto r = s.set_packet_length_setting(carbio::packet_length_setting::_128); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    case '4':
      if (auto r = s.set_packet_length_setting(carbio::packet_length_setting::_256); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
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
      s.connect("/dev/ttyAMA0");
      fmt::print("{}\n", to_json(s.get_device_settings()));
      fmt::print("device has been soft reset\n");
      return;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      return;
    }
  }
}