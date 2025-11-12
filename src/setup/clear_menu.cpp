#include "clear_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include <fmt/core.h>

#include <iostream>

void clear_menu(carbio::fingerprint_sensor& s) noexcept
{
  std::string input;
  for (;;)
  {
    fmt::print("----------\n");
    fmt::print("\nclear database:\n");
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
    case 'Y':
    case 'y':
      if (auto r = s.clear_database(); !r)
      {
        fmt::print("{}", message(r.error()));
      }
      return;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      return;
    }
  }
}