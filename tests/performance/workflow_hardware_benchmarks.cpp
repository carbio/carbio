/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *********************************************************************/

#include "fingerprint/fingerprint_sensor.h"
#include "test_utilities.h"
#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace carbio::performance_tests
{

//=============================================================================
// END-TO-END WORKFLOW BENCHMARKS
//=============================================================================

/**
 * @brief Benchmark complete enrollment workflow using convenience function
 *
 * MEASURES: Total time for complete enrollment with 12 samples (default)
 * REQUIRES: Human to place finger on sensor
 */
static void BM_HardwareWorkflow_Enrollment(benchmark::State& state)
{
  if (!is_hardware_available())
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor))
  {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  uint16_t test_location = 180; // Use IDs 180-189 for testing
  int enrollments_completed = 0;

  for (auto _ : state)
  {
    auto workflow_start = std::chrono::high_resolution_clock::now();

    if (!sensor.enroll(test_location, 12).has_value())
    {
      state.SkipWithError("Enrollment failed");
      sensor.disconnect();
      return;
    }

    auto workflow_end = std::chrono::high_resolution_clock::now();
    double total_time_ms = std::chrono::duration<double, std::milli>(workflow_end - workflow_start).count();

    state.counters["TotalWorkflow_ms"] = total_time_ms;

    enrollments_completed++;
    test_location++;
  }

  state.counters["EnrollmentsCompleted"] = enrollments_completed;

  // Cleanup: erase test templates
  for (uint16_t id = 180; id < test_location; ++id)
  {
    (void)sensor.erase_model(id, 1);
  }

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Enrollment)->Unit(benchmark::kMillisecond)->Iterations(3);

/**
 * @brief Benchmark 1:1 verification workflow using convenience function
 *
 * PREREQUISITE: Must have a template enrolled at ID 190
 * MEASURES: Authentication latency (capture to decision)
 */
static void BM_HardwareWorkflow_Verification(benchmark::State& state)
{
  if (!is_hardware_available())
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor))
  {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  uint16_t reference_id = 190;

  // Pre-loop setup: Enroll a reference template
  if (!sensor.enroll(reference_id, 12).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference template");
    return;
  }

  int successful_verifications = 0;
  int matches = 0;
  int rejections = 0;

  for (auto _ : state)
  {
    auto verification_start = std::chrono::high_resolution_clock::now();

    auto match_result = sensor.verify(reference_id);

    auto verification_end = std::chrono::high_resolution_clock::now();
    double verification_time_ms = std::chrono::duration<double, std::milli>(verification_end - verification_start).count();

    state.counters["VerificationTime_ms"] = verification_time_ms;

    if (match_result.has_value())
    {
      matches++;
      state.counters["Confidence"] = match_result->confidence.get();
    }
    else
    {
      rejections++;
    }

    successful_verifications++;
  }

  state.counters["Verifications"] = successful_verifications;
  state.counters["Matches"] = matches;
  state.counters["Rejections"] = rejections;
  state.counters["MatchRate"] = successful_verifications > 0 ? static_cast<double>(matches) / successful_verifications : 0.0;

  // Cleanup
  (void)sensor.erase_model(reference_id, 1);

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Verification)->Unit(benchmark::kMillisecond)->Iterations(5);

/**
 * @brief Benchmark 1:N identification workflow using convenience function
 *
 * PREREQUISITE: Database populated with templates
 * MEASURES: Search time across database
 */
static void BM_HardwareWorkflow_Identification(benchmark::State& state)
{
  if (!is_hardware_available())
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor))
  {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  auto count = sensor.model_count();
  uint16_t db_size = count.has_value() ? count.value() : 0;

  if (db_size == 0)
  {
    sensor.disconnect();
    state.SkipWithError("Database empty - enroll templates first");
    return;
  }

  int identifications = 0;
  int matches_found = 0;

  for (auto _ : state)
  {
    auto identification_start = std::chrono::high_resolution_clock::now();

    auto search_result = sensor.identify();

    auto identification_end = std::chrono::high_resolution_clock::now();
    double identification_time_ms = std::chrono::duration<double, std::milli>(identification_end - identification_start).count();

    state.counters["IdentificationTime_ms"] = identification_time_ms;
    state.counters["DatabaseSize"] = db_size;

    if (search_result.has_value())
    {
      matches_found++;
      state.counters["MatchIndex"] = search_result->index.get();
      state.counters["Confidence"] = search_result->confidence.get();
    }

    identifications++;
  }

  state.counters["Identifications"] = identifications;
  state.counters["MatchesFound"] = matches_found;
  state.counters["HitRate"] = identifications > 0 ? static_cast<double>(matches_found) / identifications : 0.0;

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Identification)->Unit(benchmark::kMillisecond)->Iterations(3);

} // namespace carbio::performance_tests
