#include "delete_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include "utility/conversion.h"
#include <fmt/core.h>

#include <iostream>

void delete_menu(carbio::fingerprint_sensor& s) noexcept
{
  fmt::print("----------\n");
  fmt::print("\ndelete fingerprint:\n");
  fmt::print("> specify id: ");
  std::optional<std::uint16_t> id;
  for (;;)
  {
    std::string input;
    std::getline(std::cin, input);
    if (id = carbio::parse_integral<std::uint16_t>(input); id)
    {
      break;
    }
    fmt::print("bad id\n");
  }
  if (auto r = s.erase_model(id.value()); !r)
  {
    fmt::print("{}", message(r.error()));
  }
}