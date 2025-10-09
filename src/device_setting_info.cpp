#include "carbio/device_setting_info.h"
#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio
{
device_setting_info get_default_device_setting_info() noexcept
{
  static device_setting_info instance = {
      .status = 0, .id = 0, .capacity = 127, .security_level = 3, .address = 0xFFFFFFFF, .length = 2, .baudrate = 6};
  return instance;
}

std::string to_json(const device_setting_info &info) noexcept
{
#if defined(__cpp_lib_format)
  static constexpr std::string_view json_format = "{ \"status\": {},"
                                                  " \"id\": {},"
                                                  " \"capacity\": {},"
                                                  " \"security_level\": {},"
                                                  " \"address\": {},"
                                                  " \"length\": {},"
                                                  " \"baudrate\": {} }";

  return std::format(json_format, info.status, info.id, info.capacity, info.security_level, info.address, info.length,
                     info.baudrate);
#else
  std::ostringstream ss;
  ss << "{ "
     << R"("status": )" << info.status << ", "
     << R"("id": )" << info.id << ", "
     << R"("capacity": )" << info.capacity << ", "
     << R"("security_level": )" << info.security_level << ", "
     << R"("address": )" << info.address << ", "
     << R"("length": )" << info.length << ", "
     << R"("baudrate": )" << info.baudrate << " }";
  return ss.str();
#endif
}
} // namespace carbio