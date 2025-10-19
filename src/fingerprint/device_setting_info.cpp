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

#include "carbio/fingerprint/device_setting_info.h"
#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio::fingerprint {
device_setting_info get_default_device_setting_info() noexcept {
  static device_setting_info instance = {.status = 0,
                                         .id = 0,
                                         .capacity = 127,
                                         .security_level = 3,
                                         .address = 0xFFFFFFFF,
                                         .length = 2,
                                         .baudrate = 6};
  return instance;
}

std::string to_json(const device_setting_info &info) noexcept {
#if defined(__cpp_lib_format)
  static constexpr std::string_view json_format = "{ \"status\": {},"
                                                  " \"id\": {},"
                                                  " \"capacity\": {},"
                                                  " \"security_level\": {},"
                                                  " \"address\": {},"
                                                  " \"length\": {},"
                                                  " \"baudrate\": {} }";

  return std::format(json_format, info.status, info.id, info.capacity,
                     info.security_level, info.address, info.length,
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
} // namespace carbio::fingerprint