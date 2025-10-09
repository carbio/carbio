/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
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

#ifndef CARBIO_DEVICE_INFO_H
#define CARBIO_DEVICE_INFO_H

#include <cstdint>
#include <string>
#include <string_view>

namespace carbio
{
/*!
 * @brief Device information.
 */
struct device_info
{
  std::string_view name;                 /*!< chip name */
  std::string_view manufacturer;         /*!< chip manufacturer name */
  std::string_view interface;            /*!< chip interface name */
  float            supply_voltage_min_v; /*!< chip min supply voltage */
  float            supply_voltage_max_v; /*!< chip max supply voltage */
  float            max_current_ma;       /*!< chip max current */
  float            temperature_min;      /*!< chip min operating temperature */
  float            temperature_max;      /*!< chip max operating temperature */
  std::uint32_t    driver_version;       /*!< driver version*/
};

/*!
 * @brief Retrieves the system information of the currently in-use device.
 * @returns The system information of the currently in-use device.
 */
extern device_info get_device_info() noexcept;

/*!
 * @brief Converts the system information of the current device as JSON format string.
 * @param info The current system information.
 * @returns The system information of the current device as JSON format string.
 */
extern std::string to_json(const device_info &info) noexcept;
} // namespace carbio

#endif // CARBIO_DEVICE_INFO_H
