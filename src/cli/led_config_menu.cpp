#include "led_config_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include <fmt/core.h>

#include <iostream>

void led_config_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;

  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nled config:\n");
    fmt::print("1) on\n");
    fmt::print("2) off\n");
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
      if (auto r = s.turn_led_on(); !r)
      {
        fmt::print("error: {}\n", message(r.error()));
        break;
      }
      fmt::print("-- led on\n");
      break;
    case '2':
      if (auto r = s.turn_led_off(); !r)
      {
        fmt::print("error: {}\n", message(r.error()));
        break;
      }
      fmt::print("-- led off\n");
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
    }
  }
}
