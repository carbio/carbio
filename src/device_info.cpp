#include "carbio/device_info.h"
#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio
{
device_info get_device_info() noexcept
{
  using namespace std::literals;
  static constexpr device_info instance = {.name                 = "Adafruit ID751"sv,
                                           .manufacturer         = "Adafruit"sv,
                                           .interface            = "uart"sv,
                                           .supply_voltage_min_v = 3.6f,
                                           .supply_voltage_max_v = 6.0f,
                                           .max_current_ma       = 120.0f,
                                           .temperature_min      = -20.0f,
                                           .temperature_max      = 50.0f,
                                           .driver_version       = 1000};
  return instance;
}

std::string to_json(const device_info &info) noexcept
{
#if defined(__cpp_lib_format)
  static constexpr std::string_view json_format = "{ \"name\": {},"
                                                  " \"manufacturer\": {},"
                                                  " \"interface\": {},"
                                                  " \"supply_voltage_min_v\": {},"
                                                  " \"supply_voltage_max_v\": {},"
                                                  " \"max_current_ma\": {},"
                                                  " \"temperature_min\": {},"
                                                  " \"temperature_max\": {},"
                                                  " \"driver_version\": {} }";

  return std::format(json_format, info.name, info.manufacturer, info.interface, info.supply_voltage_min_v,
                     info.supply_voltage_max_v, info.max_current_ma, info.temperature_min, info.temperature_max,
                     info.driver_version);
#else
  std::ostringstream ss;
  ss << "{"
     << R"( "name": ")" << info.name.data() << ',' << R"( "manufacturer": ")" << info.manufacturer.data() << ','
     << R"( "interface": ")" << info.interface.data() << ',' << R"( "supply_voltage_min_v": )"
     << info.supply_voltage_min_v << ',' << R"( "supply_voltage_max_v": )" << info.supply_voltage_max_v << ','
     << R"( "max_current_ma": )" << info.max_current_ma << ',' << R"( "temperature_min": )" << info.temperature_min
     << ',' << R"( "temperature_max": )" << info.temperature_max << ',' << R"( "driver_version": )"
     << info.driver_version << " }";
  return ss.str();
#endif
}
} // namespace carbio
