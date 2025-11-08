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

#include "fingerprint/device_settings.h"

#include "fingerprint/baud_rate_setting.h"
#include "fingerprint/packet_length_setting.h"
#include "fingerprint/security_level_setting.h"
#include "fingerprint/status_code.h"

#if defined(__cpp_lib_format)
#  include <format>
#else
#  include <fmt/format.h>
#endif

namespace carbio
{
std::string to_json(const device_settings& s) noexcept
{
#if defined(__cpp_lib_format)
  return std::format("{{ \"status\": \"{}\","
                     " \"id\": {},"
                     " \"capacity\": {},"
                     " \"security_level\": \"{}\","
                     " \"address\": 0x{:08X},"
                     " \"length\": \"{}\","
                     " \"baudrate\": \"{}\" }}",
                     , name(static_cast<status_code>(s.status)), s.id, s.capacity, name(static_cast<security_level_setting>(s.security_level)), s.address, name(static_cast<packet_length_setting>(s.length)),
                     name(static_cast<baud_rate_setting>(s.baudrate)));
#else
  return fmt::format(FMT_STRING("{{ \"status\": \"{}\","
                                " \"id\": {},"
                                " \"capacity\": {},"
                                " \"security_level\": \"{}\","
                                " \"address\": 0x{:08X},"
                                " \"length\": \"{}\","
                                " \"baudrate\": \"{}\" }}"),
                     name(static_cast<status_code>(s.status)), s.id, s.capacity, name(static_cast<security_level_setting>(s.security_level)), s.address, name(static_cast<packet_length_setting>(s.length)),
                     name(static_cast<baud_rate_setting>(s.baudrate)));
#endif
}
} // namespace carbio