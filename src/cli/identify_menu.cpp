#include "identify_menu.h"

#include "fingerprint/fingerprint_sensor.h"
#include <fmt/core.h>

#include <iostream>

void identify_menu(carbio::fingerprint_sensor& s) noexcept
{
  fmt::print("----------\n");
  fmt::print("\nidentification:\n");
  auto r = s.identify();
  if (!r)
  {
    fmt::print("{}", message(r.error()));
    return;
  }
  fmt::print("{}\n", carbio::to_json(r.value()));
}