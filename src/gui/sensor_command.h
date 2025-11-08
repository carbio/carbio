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

#pragma once

#include <functional>
#include <memory>

enum class CommandPriority : std::uint8_t {
  Low = 0x0,     /*!< Reserved for future use */
  High = 0x1,    /*!< Dialog operations (enroll, verify, identify) */
  Critical = 0x2 /*!< Admin authentication */
};

enum class CommandType : std::uint8_t {
  None = 0x0,            /*!< No polling */
  AdminPoll = 0x1,       /*!< Poll for administrative use cases */
  OperationalPoll = 0x2, /*!< Poll for operational use cases */
};

struct SensorCommand {
  CommandPriority priority;
  CommandType type;
  std::function<void()> execute;
  int parameter;
};

struct SensorCommandComparator {
  bool operator()(SensorCommand const &a,
                  SensorCommand const &b) const noexcept {
    // Avoid -Wstrict-overflow by ensuring unsigned comparisons
    auto const pa = static_cast<unsigned>(a.priority);
    auto const pb = static_cast<unsigned>(b.priority);
    if (pa != pb)
      return pa < pb;

    auto const ta = static_cast<unsigned>(a.type);
    auto const tb = static_cast<unsigned>(b.type);
    if (ta != tb)
      return ta < tb;

    // Use unsigned comparison to avoid signed overflow assumptions
    auto const param_a = static_cast<unsigned>(a.parameter);
    auto const param_b = static_cast<unsigned>(b.parameter);
    return param_a < param_b;
  }
};
