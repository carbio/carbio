#include "main_menu.h"

#include "clear_menu.h"
#include "delete_menu.h"
#include "device_config_menu.h"
#include "enroll_menu.h"
#include "extract_fingerprints.h"
#include "fingerprint/fingerprint_sensor.h"
#include "identify_menu.h"
#include "led_config_menu.h"
#include "verify_menu.h"
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <iostream>

void main_menu(carbio::fingerprint_sensor& s)
{
  static constexpr std::pair<char, const char*> menu_items[] = {{'e', "enroll print"}, {'i', "identify print"}, {'v', "verify print"}, {'d', "delete print"}, {'c', "clear prints"}, {'l', "led control"}, {'s', "system config"}, {'x', "quit"}};
  for (;;)
  {
    fmt::print("----------\n");
    std::array<std::uint8_t, 32> buffer{};
    auto index_result = s.read_index_table(buffer);
    if (index_result)
    {
      const auto& bmp = *index_result;
      auto fingerprint_ids = extract_fingerprints(bmp);
      if (fingerprint_ids.empty())
      {
        fmt::print("templates: []\n");
      }
      else
      {
        fmt::print("templates: [{}]\n", fmt::join(fingerprint_ids, ", "));
      }
    }
    else
    {
      fmt::print("failed to read templates");
    }

    // Render menu in one shot
    std::string menu_text;
    menu_text.reserve(256);
    for (auto [key, desc] : menu_items)
    {
      menu_text += key;
      menu_text += ") ";
      menu_text += desc;
      menu_text += '\n';
    }
    // Trim final newline
    fmt::print("\n{}", menu_text);
    fmt::print("----------\n");
    fmt::print("> ");

    std::string input;
    if (!std::getline(std::cin, input))
    {
      fmt::print("input stream closed; exiting menu\n");
      break;
    }
    if (input.empty())
    {
      continue;
    }

    const char cmd = input.front();
    if (cmd == 'x' || cmd == 'X')
    {
      fmt::print("quitting...\n");
      break;
    }

    switch (cmd)
    {
    case 'e':
      enroll_menu(s);
      break;
    case 'i':
      identify_menu(s);
      break;
    case 'v':
      verify_menu(s);
      break;
    case 'd':
      delete_menu(s);
      break;
    case 'c':
      clear_menu(s);
      break;
    case 'l':
      led_config_menu(s);
      break;
    case 's':
      device_config_menu(s);
      break;
    default:
      fmt::print("unknown command '{}'\n", cmd);
      break;
    }
  }
}