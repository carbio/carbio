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

#include "carbio/fingerprint/device_info.h"

#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio::fingerprint {
device_info get_device_info() noexcept {
  using namespace std::literals;
  static constexpr device_info instance = {.name = "Adafruit ID751",
                                           .manufacturer = "Adafruit",
                                           .interface = "uart",
                                           .supply_voltage_min_v = 3.6f,
                                           .supply_voltage_max_v = 6.0f,
                                           .max_current_ma = 120.0f,
                                           .temperature_min = -20.0f,
                                           .temperature_max = 50.0f,
                                           .driver_version = 1000};
  return instance;
}

std::string to_json(const device_info &info) noexcept {
#if defined(__cpp_lib_format)
  static constexpr std::string_view json_format =
      "{ \"name\": {},"
      " \"manufacturer\": {},"
      " \"interface\": {},"
      " \"supply_voltage_min_v\": {},"
      " \"supply_voltage_max_v\": {},"
      " \"max_current_ma\": {},"
      " \"temperature_min\": {},"
      " \"temperature_max\": {},"
      " \"driver_version\": {} }";

  return std::format(json_format, info.name, info.manufacturer, info.interface,
                     info.supply_voltage_min_v, info.supply_voltage_max_v,
                     info.max_current_ma, info.temperature_min,
                     info.temperature_max, info.driver_version);
#else
  std::ostringstream ss;
  ss << "{"
     << R"( "name": ")" << info.name << ',' << R"( "manufacturer": ")"
     << info.manufacturer << ',' << R"( "interface": ")"
     << info.interface << ',' << R"( "supply_voltage_min_v": )"
     << info.supply_voltage_min_v << ',' << R"( "supply_voltage_max_v": )"
     << info.supply_voltage_max_v << ',' << R"( "max_current_ma": )"
     << info.max_current_ma << ',' << R"( "temperature_min": )"
     << info.temperature_min << ',' << R"( "temperature_max": )"
     << info.temperature_max << ',' << R"( "driver_version": )"
     << info.driver_version << " }";
  return ss.str();
#endif
}
} // namespace carbio::fingerprint
