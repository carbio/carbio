/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * SECURITY-CRITICAL BENCHMARKS
 *
 * These benchmarks measure security-relevant performance characteristics:
 *
 * 1. Confidence score distributions (detect anomalies)
 * 2. Security level impact on performance
 * 3. Match threshold sensitivity
 * 4. Authentication decision latency
 * 5. Template quality metrics
 *********************************************************************/

#include "fingerprint/fingerprint_sensor.h"
#include "test_utilities.h"
#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <thread>
#include <vector>

namespace carbio::performance_tests
{

//=============================================================================
// SECURITY: CONFIDENCE SCORE DISTRIBUTION ANALYSIS
//
// Confidence scores are THE critical security metric.
// Low variance = good template quality
// High variance = poor capture or spoofing attempt
//=============================================================================

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

  // Pre-loop setup: Enroll reference template at ID 195 (timer hasn't started)
  uint16_t reference_id = 195;

  for (int attempt = 0; attempt < 2; ++attempt)
  {
    spdlog::info("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 100; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        spdlog::info("Captured");
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
      spdlog::info("Remove finger");
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(reference_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference");
    return;
  }

  std::vector<uint32_t> confidence_scores;

  for (auto _ : state)
  {
    {
      benchmark_pause_guard pause(state);
      spdlog::info("Place finger on sensor");
    }

    // Capture
    bool captured = false;
    for (int retry = 0; retry < 100 && !captured; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        {
          benchmark_pause_guard pause(state);
          spdlog::info("Captured");
        }
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

    auto match = sensor.match_model();
    if (match.has_value())
    {
      confidence_scores.push_back(match->confidence);
    }
    else
    {
      // Record rejection as confidence = 0
      confidence_scores.push_back(0);
    }
  }

  // Statistical analysis
  if (!confidence_scores.empty())
  {
    double sum = std::accumulate(confidence_scores.begin(), confidence_scores.end(), 0.0);
    double mean = sum / confidence_scores.size();

    double sq_sum = std::inner_product(confidence_scores.begin(), confidence_scores.end(), confidence_scores.begin(), 0.0);
    double stddev = std::sqrt(sq_sum / confidence_scores.size() - mean * mean);

    auto [min_it, max_it] = std::minmax_element(confidence_scores.begin(), confidence_scores.end());

    state.counters["Confidence_Mean"] = mean;
    state.counters["Confidence_StdDev"] = stddev;
    state.counters["Confidence_Min"] = *min_it;
    state.counters["Confidence_Max"] = *max_it;
    state.counters["Confidence_CV"] = (stddev / mean) * 100.0; // Coefficient of variation %

    int rejections = std::count(confidence_scores.begin(), confidence_scores.end(), 0);
    state.counters["FalseRejectCount"] = rejections;
    state.counters["FalseRejectRate"] = static_cast<double>(rejections) / confidence_scores.size();
  }

  (void)sensor.erase_model(reference_id, 1); // Suppress [[nodiscard]] warning
  sensor.disconnect();
}
BENCHMARK(BM_Security_MatchConfidenceDistribution)->Unit(benchmark::kMillisecond)->Iterations(10); // 10 match attempts for statistical significance

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

  std::vector<uint32_t> confidence_scores;
  int matches = 0;
  int not_found = 0;

  for (auto _ : state)
  {
    {
      benchmark_pause_guard pause(state);
      spdlog::info("Place finger on sensor");
    }

    bool captured = false;
    for (int retry = 0; retry < 100 && !captured; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        {
          benchmark_pause_guard pause(state);
          spdlog::info("Captured");
        }
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

    auto search = sensor.fast_search_model(0, 1, db_size);
    if (search.has_value())
    {
      confidence_scores.push_back(search->confidence);
      matches++;
    }
    else
    {
      not_found++;
    }
  }

  // Statistical analysis
  if (!confidence_scores.empty())
  {
    double sum = std::accumulate(confidence_scores.begin(), confidence_scores.end(), 0.0);
    double mean = sum / confidence_scores.size();

    double sq_sum = std::inner_product(confidence_scores.begin(), confidence_scores.end(), confidence_scores.begin(), 0.0);
    double stddev = std::sqrt(sq_sum / confidence_scores.size() - mean * mean);

    auto [min_it, max_it] = std::minmax_element(confidence_scores.begin(), confidence_scores.end());

    state.counters["Confidence_Mean"] = mean;
    state.counters["Confidence_StdDev"] = stddev;
    state.counters["Confidence_Min"] = *min_it;
    state.counters["Confidence_Max"] = *max_it;
    state.counters["Confidence_CV"] = (stddev / mean) * 100.0;
  }

  state.counters["DatabaseSize"] = db_size;
  state.counters["Matches"] = matches;
  state.counters["NotFound"] = not_found;
  state.counters["HitRate"] = static_cast<double>(matches) / (matches + not_found);

  sensor.disconnect();
}
BENCHMARK(BM_Security_SearchConfidenceDistribution)->Unit(benchmark::kMillisecond)->Iterations(10);

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
  uint16_t test_id = 196;
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    spdlog::info("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        spdlog::info("Captured");
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
      spdlog::info("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(test_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll at security level " + std::to_string(security_level));
    return;
  }

  std::vector<double> latencies_ms;
  std::vector<uint32_t> confidences;
  int matches = 0;
  int rejections = 0;

  for (auto _ : state)
  {
    {
      benchmark_pause_guard pause(state);
      spdlog::info("Place finger on sensor");
    }

    // Capture
    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        {
          benchmark_pause_guard pause(state);
          spdlog::info("Captured");
        }
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

    (void)sensor.extract_features(1);    // Suppress [[nodiscard]] warning
    (void)sensor.load_model(test_id, 2); // Suppress [[nodiscard]] warning

    auto start = std::chrono::high_resolution_clock::now();
    auto match = sensor.match_model();
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (match.has_value())
    {
      matches++;
      confidences.push_back(match->confidence);
    }
    else
    {
      rejections++;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());

  state.counters["SecurityLevel"] = security_level;
  state.counters["MatchLatency_P50_ms"] = latencies_ms[latencies_ms.size() / 2];
  state.counters["Matches"] = matches;
  state.counters["Rejections"] = rejections;
  state.counters["MatchRate"] = static_cast<double>(matches) / (matches + rejections);

  if (!confidences.empty())
  {
    double mean_conf = std::accumulate(confidences.begin(), confidences.end(), 0.0) / confidences.size();
    state.counters["Confidence_Mean"] = mean_conf;
  }

  (void)sensor.erase_model(test_id, 1); // Suppress [[nodiscard]] warning
  sensor.disconnect();
}
BENCHMARK(BM_Security_SecurityLevelImpact)
    ->Arg(1) // Lowest security
    ->Arg(2) // Low
    ->Arg(3) // Balanced
    ->Arg(4) // High
    ->Arg(5) // Highest security
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10);

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
  uint16_t auth_id = 197;
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    spdlog::info("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        spdlog::info("Captured");
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
      spdlog::info("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(auth_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to enroll reference");
    return;
  }

  std::vector<double> decision_times_ms;
  int granted = 0;
  int denied = 0;

  for (auto _ : state)
  {
    {
      benchmark_pause_guard pause(state);
      spdlog::info("Place finger on sensor");
    }

    // COMPLETE authentication workflow timing
    auto decision_start = std::chrono::high_resolution_clock::now();

    // 1. Wait for finger
    bool finger_detected = false;
    for (int retry = 0; retry < 100; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        finger_detected = true;
        {
          benchmark_pause_guard pause(state);
          spdlog::info("Captured");
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!finger_detected)
    {
      state.SkipWithError("Finger detection timeout");
      break;
    }

    // 2. Extract features
    if (!sensor.extract_features(1).has_value())
    {
      state.SkipWithError("Extract failed");
      break;
    }

    // 3. Load stored template
    if (!sensor.load_model(auth_id, 2).has_value())
    {
      state.SkipWithError("Load failed");
      break;
    }

    // 4. Match and decide
    auto match = sensor.match_model();

    auto decision_end = std::chrono::high_resolution_clock::now();
    double decision_time = std::chrono::duration<double, std::milli>(decision_end - decision_start).count();
    decision_times_ms.push_back(decision_time);

    if (match.has_value() && match->confidence > 100)
    {
      granted++;
      state.counters["LastConfidence"] = match->confidence.get();
    }
    else
    {
      denied++;
    }
  }

  std::sort(decision_times_ms.begin(), decision_times_ms.end());
  size_t n = decision_times_ms.size();

  // CRITICAL: P95 and P99 latencies for SLA compliance
  state.counters["DecisionTime_P50_ms"] = decision_times_ms[n / 2];
  state.counters["DecisionTime_P95_ms"] = decision_times_ms[n * 95 / 100];
  state.counters["DecisionTime_P99_ms"] = decision_times_ms[n * 99 / 100];
  state.counters["DecisionTime_Max_ms"] = decision_times_ms.back();

  state.counters["AccessGranted"] = granted;
  state.counters["AccessDenied"] = denied;
  state.counters["GrantRate"] = static_cast<double>(granted) / (granted + denied);

  (void)sensor.erase_model(auth_id, 1); // Suppress [[nodiscard]] warning
  sensor.disconnect();
}
BENCHMARK(BM_Security_AuthenticationDecisionLatency)->Unit(benchmark::kMillisecond)->Iterations(10);

} // namespace carbio::performance_tests
