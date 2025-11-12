/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
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

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>
#include <iostream>

namespace carbio::performance_tests
{
// Forward declarations for hardware benchmark initialization
void initialize_hardware_benchmarks();
void cleanup_hardware_benchmarks();
} // namespace carbio::performance_tests

int main(int argc, char** argv)
{
  spdlog::flush_on(spdlog::level::info);
  spdlog::flush_every(std::chrono::seconds(0));
  spdlog::set_pattern("%v");

  std::cout.setf(std::ios::unitbuf);
  std::cerr.setf(std::ios::unitbuf);
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  spdlog::set_level(spdlog::level::warn);
  carbio::performance_tests::initialize_hardware_benchmarks();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
  {
    carbio::performance_tests::cleanup_hardware_benchmarks();
    return 1;
  }

  int result = ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();

  // Cleanup hardware benchmarks global state
  carbio::performance_tests::cleanup_hardware_benchmarks();

  return result;
}
