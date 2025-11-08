#include "verify_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include "utility/conversion.h"
#include <fmt/core.h>

#include <iostream>

void verify_menu(carbio::fingerprint_sensor& s) noexcept
{
  fmt::print("----------\n");
  fmt::print("\nverification:\n");
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
  auto r = s.verify(id.value());
  if (!r)
  {
    fmt::print("{}", message(r.error()));
    return;
  }
  fmt::print("{}\n", carbio::to_json(r.value()));
}