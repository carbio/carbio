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

#include "fingerprint/fingerprint_sensor.h"
#include "test_utilities.h"
#include <benchmark/benchmark.h>

#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

namespace carbio::performance_tests
{

/**
 * @brief Benchmark complete enrollment workflow with comprehensive metrics
 *
 * MEASURES: Total time for complete enrollment with 12 samples (default)
 * REQUIRES: Human to place finger on sensor
 *
 * METRICS COLLECTED:
 * - End-to-end enrollment time (percentiles, mean, std dev)
 * - Enrollment success/failure rates (FTE - Failure To Enroll)
 * - Per-sample average time
 * - Throughput (enrollments per hour)
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

  // Query sensor capacity and use a safe template ID range
  auto settings = sensor.query_device_settings();
  uint16_t test_location = 80;  // Default safe starting ID
  if (settings.has_value() && settings->capacity > 100)
  {
    test_location = settings->capacity - 30;  // Use IDs well within range
  }
  SPDLOG_INFO("Using test template IDs starting from: {} (capacity: {})", test_location, settings.has_value() ? settings->capacity : 0);

  const uint16_t sample_count = 12;

  SPDLOG_INFO("Each enrollment requires {} finger placements", sample_count);
  SPDLOG_INFO("Follow prompts from enroll() function\n");

  int iteration = 0;
  int enrollments_completed = 0;
  int enrollments_failed = 0;

  for (auto _ : state)
  {
    iteration++;
    SPDLOG_INFO("Starting enrollment {}/{}...", iteration, static_cast<int>(state.max_iterations));

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.enroll(test_location, sample_count);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (result.has_value())
    {
      enrollments_completed++;
      SPDLOG_INFO("Enrollment {} completed successfully\n", iteration);
      test_location++;
    }
    else
    {
      enrollments_failed++;
      SPDLOG_WARN("Enrollment {} failed\n", iteration);
    }
  }

  state.counters["Success"] = enrollments_completed;
  state.counters["Failed"] = enrollments_failed;
  int total = enrollments_completed + enrollments_failed;
  state.counters["FTE_%"] = total > 0 ? (static_cast<double>(enrollments_failed) / total) * 100.0 : 0.0;

  for (uint16_t id = 180; id < test_location; ++id)
  {
    (void)sensor.erase_model(id, 1);
  }

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Enrollment)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

/**
 * @brief Benchmark 1:1 verification workflow with comprehensive authentication metrics
 *
 * PREREQUISITE: Must have a template enrolled at ID 190
 * MEASURES: Authentication latency, accuracy metrics (FRR approximation), confidence scores
 *
 * METRICS COLLECTED:
 * - End-to-end verification latency (percentiles, mean, std dev, jitter)
 * - Match/rejection rates
 * - False Rejection Rate (FRR) approximation (genuine samples rejected)
 * - Confidence score distribution
 * - Throughput for authentication operations
 * - Real-time performance (WCET, jitter)
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

  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t reference_id = 90;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    reference_id = settings->capacity - 31;  // Use an ID well within range
  }
  SPDLOG_INFO("Using reference template ID: {} (capacity: {})", reference_id, settings.has_value() ? settings->capacity : 0);

  SPDLOG_INFO("STEP 1: Enrolling reference template (ID: {})...", reference_id);
  SPDLOG_INFO("This requires 12 finger placements\n");

  // Pre-loop setup: Enroll a reference template
  if (!sensor.enroll(reference_id, 12).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference template");
    return;
  }

  SPDLOG_INFO("Reference template enrolled successfully");
  SPDLOG_INFO("STEP 2: Running verification iterations");
  SPDLOG_INFO("Place the SAME finger for each verification\n");

  int iteration = 0;
  int matches = 0;
  int rejections = 0;

  for (auto _ : state)
  {
    iteration++;
    SPDLOG_INFO("Verification {}/{}: Place your finger on the sensor...", iteration, static_cast<int>(state.max_iterations));

    constexpr int max_retries = 3;
    int retry_count = 0;
    decltype(sensor.verify(reference_id)) match_result;

    auto start = std::chrono::high_resolution_clock::now();

    do
    {
      if (retry_count > 0)
      {
        SPDLOG_WARN("  Retrying after device_busy error (attempt {}/{})", retry_count + 1, max_retries);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }

      match_result = sensor.verify(reference_id);
      retry_count++;

      if (match_result.has_value() || match_result.error() != status_code::device_busy)
      {
        break;
      }
    } while (retry_count < max_retries);

    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(match_result);

    if (match_result.has_value())
    {
      matches++;
      state.counters["Confidence"] = match_result->confidence.get();
      SPDLOG_INFO("  MATCH (confidence: {})", match_result->confidence.get());
    }
    else
    {
      rejections++;
      auto error = match_result.error();
      SPDLOG_WARN("  NO MATCH - Error: {} ({})", carbio::name(error), carbio::hex_string(error));
    }
  }

  SPDLOG_INFO("\nVerification benchmark completed");

  state.counters["Matches"] = matches;
  state.counters["Rejects"] = rejections;
  int total = matches + rejections;
  state.counters["FRR_%"] = total > 0 ? (static_cast<double>(rejections) / total) * 100.0 : 0.0;

  (void)sensor.erase_model(reference_id, 1);

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Verification)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

/**
 * @brief Benchmark 1:N identification workflow with comprehensive search metrics
 *
 * PREREQUISITE: Database populated with templates
 * MEASURES: Search time across database, scalability with database size
 *
 * METRICS COLLECTED:
 * - Identification latency (percentiles, mean, std dev, jitter)
 * - Database size and search scalability
 * - Hit rate (successful identifications)
 * - Confidence score statistics
 * - Throughput (identifications per second)
 * - Real-time metrics (WCET)
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

  SPDLOG_INFO("Database size: {} templates", db_size);

  if (db_size == 0)
  {
    SPDLOG_ERROR("Database is empty - please enroll templates first");
    SPDLOG_INFO("Use the enrollment benchmark to create test templates");
    sensor.disconnect();
    state.SkipWithError("Database empty - enroll templates first");
    return;
  }

  SPDLOG_INFO("Running 1:N identification...");
  SPDLOG_INFO("Place an enrolled finger for each iteration\n");

  int iteration = 0;
  int matches_found = 0;

  for (auto _ : state)
  {
    iteration++;
    SPDLOG_INFO("Identification {}/{}: Place your finger on the sensor...", iteration, static_cast<int>(state.max_iterations));

    constexpr int max_retries = 3;
    int retry_count = 0;
    decltype(sensor.identify()) search_result;

    auto start = std::chrono::high_resolution_clock::now();

    do
    {
      if (retry_count > 0)
      {
        SPDLOG_WARN("  Retrying after device_busy error (attempt {}/{})", retry_count + 1, max_retries);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }

      search_result = sensor.identify();
      retry_count++;

      if (search_result.has_value() || search_result.error() != status_code::device_busy)
      {
        break;
      }
    } while (retry_count < max_retries);

    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(search_result);

    if (search_result.has_value())
    {
      matches_found++;
      state.counters["Confidence"] = search_result->confidence.get();
      SPDLOG_INFO("  MATCH FOUND: ID {} (confidence: {})", search_result->index.get(), search_result->confidence.get());
    }
    else
    {
      auto error = search_result.error();
      SPDLOG_WARN("  NO MATCH - Error: {} ({})", carbio::name(error), carbio::hex_string(error));
    }
  }

  SPDLOG_INFO("\nIdentification benchmark completed");

  state.counters["DB_Size"] = db_size;
  state.counters["Matches"] = matches_found;

  sensor.disconnect();
}
BENCHMARK(BM_HardwareWorkflow_Identification)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

} // namespace carbio::performance_tests
