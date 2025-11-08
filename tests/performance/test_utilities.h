/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * PERFORMANCE TEST UTILITIES
 *
 * Shared utility functions for hardware-based performance tests
 *********************************************************************/

#ifndef CARBIO_TESTS_PERFORMANCE_TEST_UTILITIES_H
#define CARBIO_TESTS_PERFORMANCE_TEST_UTILITIES_H

#include "fingerprint/fingerprint_sensor.h"

#include <benchmark/benchmark.h>

namespace carbio::performance_tests {

/**
 * @brief Check if R30x hardware sensor is available
 * @return true if sensor is detected and accessible
 */
bool is_hardware_available();

/**
 * @brief Open and initialize the fingerprint sensor
 * @param sensor Reference to sensor object to initialize
 * @return true if sensor opened successfully
 */
bool open_sensor(carbio::fingerprint_sensor& sensor);

//=============================================================================
// RAII BENCHMARK TIMING GUARD - Automotive-grade implementation
//=============================================================================

/**
 * @brief RAII guard for pausing benchmark timing during I/O operations
 *
 * SAFETY GUARANTEES:
 * - Exception-safe: Timing always resumes via RAII
 * - Non-copyable: Prevents accidental double-resume
 * - Non-movable: Prevents timing state corruption
 * - Scope-based: Self-documenting and automatic cleanup
 *
 * USAGE (inside benchmark loop only):
 * @code
 * for (auto _ : state) {
 *   {
 *     benchmark_pause_guard pause(state);
 *     // I/O operations here (user interaction, file access)
 *     spdlog::info("Place finger on sensor");
 *   } // Timing automatically resumes here
 *
 *   // Timed benchmark code here
 *   auto result = sensor.capture_image();
 * }
 * @endcode
 *
 * PRECONDITION: Must be used INSIDE the benchmark loop (after 'for (auto _ : state)')
 * RATIONALE: Google Benchmark timer doesn't start until loop entry
 *
 * @note For pre-loop setup, just write normal code - timer hasn't started yet!
 */
class [[nodiscard]] benchmark_pause_guard {
public:
  /**
   * @brief Construct guard and pause benchmark timing
   * @param state Benchmark state (must be inside iteration loop)
   */
  explicit benchmark_pause_guard(benchmark::State& state) noexcept
    : state_(state) {
    state_.PauseTiming();
  }

  /**
   * @brief Destroy guard and resume benchmark timing
   */
  ~benchmark_pause_guard() noexcept {
    state_.ResumeTiming();
  }

  // Non-copyable: Prevents double-resume timing corruption
  benchmark_pause_guard(const benchmark_pause_guard&) = delete;
  benchmark_pause_guard& operator=(const benchmark_pause_guard&) = delete;

  // Non-movable: Moving a timing guard has no sensible semantics
  benchmark_pause_guard(benchmark_pause_guard&&) = delete;
  benchmark_pause_guard& operator=(benchmark_pause_guard&&) = delete;

private:
  benchmark::State& state_;
};

} // namespace carbio::performance_tests

#endif // CARBIO_TESTS_PERFORMANCE_TEST_UTILITIES_H
