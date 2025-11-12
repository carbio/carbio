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
 * @brief Benchmark match confidence score distribution
 *
 * SECURITY CRITICAL: Analyze genuine match confidence patterns
 * MEASURES: Mean, std dev, min, max of match confidence scores
 * REQUIRES: Same finger scanned multiple times
 */
static void BM_Security_MatchConfidenceDistribution(benchmark::State& state)
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

  // Pre-loop setup: Enroll reference template (timer hasn't started)
  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t reference_id = 95;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    reference_id = settings->capacity - 20;  // Use an ID well within range
  }
  SPDLOG_INFO("Using reference template ID: {} (capacity: {})", reference_id, settings.has_value() ? settings->capacity : 0);

  for (int attempt = 0; attempt < 2; ++attempt)
  {
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 100; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!captured)
    {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    (void)sensor.extract_features(attempt + 1); // Suppress [[nodiscard]] warning

    if (attempt == 0)
    {
      SPDLOG_INFO("Remove finger");
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(reference_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference");
    return;
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 100 && !captured; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!captured)
    {
      state.SkipWithError("Capture timeout");
      break;
    }

    if (!sensor.extract_features(1).has_value())
    {
      state.SkipWithError("Extract failed");
      break;
    }

    if (!sensor.load_model(reference_id, 2).has_value())
    {
      state.SkipWithError("Load failed");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto match = sensor.match_model();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(match);

    if (match.has_value())
    {
      state.counters["Confidence"] = match->confidence.get();
    }
  }

  (void)sensor.erase_model(reference_id, 1);
  sensor.disconnect();
}
BENCHMARK(BM_Security_MatchConfidenceDistribution)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

/**
 * @brief Benchmark search confidence score distribution
 *
 * SECURITY CRITICAL: Analyze 1:N search confidence patterns
 * MEASURES: Confidence scores from database searches
 */
static void BM_Security_SearchConfidenceDistribution(benchmark::State& state)
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
  if (!count.has_value() || count.value() < 10)
  {
    sensor.disconnect();
    state.SkipWithError("Need at least 10 templates in database");
    return;
  }

  uint16_t db_size = count.value();

  for (auto _ : state)
  {
    state.PauseTiming();
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 100 && !captured; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!captured)
    {
      state.SkipWithError("Capture timeout");
      break;
    }

    if (!sensor.extract_features(1).has_value())
    {
      state.SkipWithError("Extract failed");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto search = sensor.fast_search_model(0, 1, db_size);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(search);

    if (search.has_value())
    {
      state.counters["Confidence"] = search->confidence.get();
    }
  }

  state.counters["DatabaseSize"] = db_size;

  sensor.disconnect();
}
BENCHMARK(BM_Security_SearchConfidenceDistribution)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

//=============================================================================
// SECURITY: SECURITY LEVEL IMPACT
//
// Higher security levels increase match accuracy but also latency
//=============================================================================

/**
 * @brief Benchmark performance impact of security level settings
 *
 * SECURITY CRITICAL: Trade-off between security and performance
 * MEASURES: Latency and confidence at different security levels
 */
static void BM_Security_SecurityLevelImpact(benchmark::State& state)
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

  int security_level = state.range(0); // 1-5

  // Set security level
  auto level_setting = static_cast<security_level_setting>(security_level);
  if (!sensor.set_security_level_setting(level_setting).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to set security level");
    return;
  }

  // Pre-loop setup: Enroll reference at security level (timer hasn't started)
  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t test_id = 96;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    test_id = settings->capacity - 21;  // Use an ID well within range
  }
  SPDLOG_INFO("Using test template ID: {} (capacity: {})", test_id, settings.has_value() ? settings->capacity : 0);
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    (void)sensor.extract_features(attempt + 1); // Suppress [[nodiscard]] warning

    if (attempt == 0)
    {
      SPDLOG_INFO("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(test_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll at security level " + std::to_string(security_level));
    return;
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    (void)sensor.extract_features(1);
    (void)sensor.load_model(test_id, 2);
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto match = sensor.match_model();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(match);

    if (match.has_value())
    {
      state.counters["Confidence"] = match->confidence.get();
    }
  }

  state.counters["SecurityLevel"] = security_level;

  (void)sensor.erase_model(test_id, 1);
  sensor.disconnect();
}
BENCHMARK(BM_Security_SecurityLevelImpact)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Arg(5)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->MinTime(5.0)
    ->MinWarmUpTime(1.0);

//=============================================================================
// SECURITY: AUTHENTICATION DECISION TIME
//
// Critical for real-time access control systems
//=============================================================================

/**
 * @brief Benchmark complete authentication decision latency
 *
 * SECURITY CRITICAL: Time from sensor touch to access granted/denied
 * MEASURES: End-to-end authentication latency (capture + extract + load + match)
 */
static void BM_Security_AuthenticationDecisionLatency(benchmark::State& state)
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

  // Pre-loop setup: Enroll reference (timer hasn't started)
  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t auth_id = 97;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    auth_id = settings->capacity - 22;  // Use an ID well within range
  }
  SPDLOG_INFO("Using auth template ID: {} (capacity: {})", auth_id, settings.has_value() ? settings->capacity : 0);
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    SPDLOG_INFO("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    (void)sensor.extract_features(attempt + 1); // Suppress [[nodiscard]] warning

    if (attempt == 0)
    {
      SPDLOG_INFO("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(auth_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference");
    return;
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    SPDLOG_INFO("Place finger on sensor");

    bool finger_detected = false;
    for (int retry = 0; retry < 100; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        finger_detected = true;
        SPDLOG_INFO("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!finger_detected)
    {
      state.SkipWithError("Finger detection timeout");
      break;
    }

    if (!sensor.extract_features(1).has_value())
    {
      state.SkipWithError("Extract failed");
      break;
    }

    if (!sensor.load_model(auth_id, 2).has_value())
    {
      state.SkipWithError("Load failed");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto match = sensor.match_model();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(match);

    if (match.has_value())
    {
      state.counters["Confidence"] = match->confidence.get();
    }
  }

  (void)sensor.erase_model(auth_id, 1);
  sensor.disconnect();
}
BENCHMARK(BM_Security_AuthenticationDecisionLatency)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

} // namespace carbio::performance_tests
