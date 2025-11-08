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
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace carbio::performance_tests
{

/**
 * @brief Hardware configuration for R30x sensor
 */
struct hardware_config
{
  static constexpr const char* device_path = "/dev/ttyAMA0";     // Raspberry Pi UART
  static constexpr const char* device_path_usb = "/dev/ttyUSB0"; // USB adapter fallback
  static constexpr bool skip_if_unavailable = true;
};

/**
 * @brief Check if real hardware is available (cached for performance)
 */
bool is_hardware_available()
{
  static bool checked = false;
  static bool available = false;

  if (!checked)
  {
    fingerprint_sensor sensor;

    // Try primary device
    if (sensor.connect(hardware_config::device_path))
    {
      auto info = sensor.query_device_settings();
      sensor.disconnect();
      available = info.has_value();
      checked = true;
      return available;
    }

    // Try USB fallback
    if (sensor.connect(hardware_config::device_path_usb))
    {
      auto info = sensor.query_device_settings();
      sensor.disconnect();
      available = info.has_value();
      checked = true;
      return available;
    }

    checked = true;
    available = false;
  }

  return available;
}

/**
 * @brief Open sensor on any available device
 */
bool open_sensor(fingerprint_sensor& sensor)
{
  if (sensor.connect(hardware_config::device_path))
  {
    return true;
  }
  if (sensor.connect(hardware_config::device_path_usb))
  {
    return true;
  }
  return false;
}

//=============================================================================
// BENCHMARK FIXTURE - Shared sensor connection for all benchmarks
//=============================================================================

/**
 * @brief Benchmark fixture that maintains a single sensor connection
 *
 * PERFORMANCE OPTIMIZATION:
 * - Connection established once during SetUp (before all iterations)
 * - Shared across all benchmark iterations
 * - Disconnected once during TearDown (after all iterations)
 * - Eliminates 50-60ms connection overhead per benchmark
 *
 * SAFETY:
 * - Thread-safe: Only thread 0 manages the connection
 * - Exception-safe: RAII guarantees cleanup
 * - Skip on hardware failure
 */
class SensorFixture : public benchmark::Fixture
{
protected:
  fingerprint_sensor sensor;
  bool hardware_ready = false;

public:
  void SetUp(const ::benchmark::State& state) override
  {
    // Only the first thread should set up the sensor
    if (state.thread_index() == 0)
    {
      if (is_hardware_available() && open_sensor(sensor))
      {
        hardware_ready = true;
      }
    }
  }

  void TearDown(const ::benchmark::State& state) override
  {
    // Only the first thread should tear down the sensor
    if (state.thread_index() == 0 && hardware_ready)
    {
      sensor.disconnect();
      hardware_ready = false;
    }
  }
};

//=============================================================================
// COMMAND LATENCY BENCHMARKS - Measure actual hardware response times
//=============================================================================

/**
 * @brief Benchmark actual image capture latency
 *
 * MEASURES: Wall-clock time from command to response
 * REQUIRES: Physical R30x sensor
 * INTERACTION: None (automated polling)
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_CaptureImageLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available - connect hardware to run this benchmark");
    return;
  }

  std::vector<double> latencies_ms;
  int successful = 0;
  int no_finger = 0;
  int other_errors = 0;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.capture_image();
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (result.has_value())
    {
      successful++;
    }
    else if (result.error() == status_code::no_finger)
    {
      no_finger++;
    }
    else
    {
      other_errors++;
    }
  }

  // Report percentiles (critical for automotive real-time requirements)
  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n * 50 / 100];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Latency_P99_ms"] = latencies_ms[n * 99 / 100];
  state.counters["Latency_Min_ms"] = latencies_ms.front();
  state.counters["Latency_Max_ms"] = latencies_ms.back();

  state.counters["Successful"] = successful;
  state.counters["NoFinger"] = no_finger;
  state.counters["Errors"] = other_errors;
  state.counters["SuccessRate"] = static_cast<double>(successful) / n;
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_CaptureImageLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(100);

/**
 * @brief Benchmark feature extraction latency
 *
 * PREREQUISITE: Must capture image first
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_ExtractFeaturesLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  // Prerequisite: capture one valid image
  bool image_captured = false;
  for (int retry = 0; retry < 50 && !image_captured; ++retry)
  {
    if (sensor.capture_image().has_value())
    {
      image_captured = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (!image_captured)
  {
    state.SkipWithError("Timeout waiting for finger - no image captured after 5 seconds");
    return;
  }

  std::vector<double> latencies_ms;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.extract_features(1);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (!result.has_value())
    {
      state.SkipWithError("Extract features failed");
      return;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Latency_P99_ms"] = latencies_ms[n * 99 / 100];
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_ExtractFeaturesLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(50);

/**
 * @brief Benchmark model creation latency
 *
 * PREREQUISITE: Must have features in buffers 1 and 2
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_CreateModelLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  // Pre-loop setup: Extract features to buffer 1 and 2 (timer hasn't started)
  bool ready = false;
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    // Wait for finger
    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      state.SkipWithError("Timeout waiting for finger (5 seconds)");
      return;
    }

    if (!sensor.extract_features(attempt + 1).has_value())
    {
      state.SkipWithError("Failed to extract features for prerequisite");
      return;
    }

    if (attempt == 0)
    {
      // Wait for finger removal before second capture
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    else
    {
      ready = true;
    }
  }

  if (!ready)
  {
    state.SkipWithError("Failed to prepare buffers");
    return;
  }

  std::vector<double> latencies_ms;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.merge_model();
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (!result.has_value())
    {
      state.SkipWithError("Create model failed");
      return;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_CreateModelLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(20);

/**
 * @brief Benchmark template storage latency
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_StoreModelLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  // Pre-loop setup: Create a model to store (timer hasn't started)
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      state.SkipWithError("Timeout waiting for finger (5 seconds)");
      return;
    }

    if (!sensor.extract_features(attempt + 1).has_value())
    {
      state.SkipWithError("Failed to extract features");
      return;
    }

    if (attempt == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  if (!sensor.merge_model().has_value())
  {
    state.SkipWithError("Failed to create model");
    return;
  }

  std::vector<double> latencies_ms;
  uint16_t test_location = 200; // Use high IDs to avoid conflicts

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.store_model(test_location);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (!result.has_value())
    {
      state.SkipWithError("Store model failed");
      return;
    }

    test_location++;
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];

  // Cleanup: erase test templates
  for (uint16_t id = 200; id < test_location; ++id)
  {
    (void)sensor.erase_model(id, 1); // Suppress [[nodiscard]] warning
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_StoreModelLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(10);

/**
 * @brief Benchmark template loading latency
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_LoadModelLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  // Pre-loop setup: Store a template at location 250 (timer hasn't started)
  uint16_t test_id = 250;

  // Create and store a test template
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    bool captured = false;
    for (int retry = 0; retry < 50; ++retry)
    {
      if (sensor.capture_image().has_value())
      {
        captured = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured)
    {
      state.SkipWithError("Timeout waiting for finger (5 seconds)");
      return;
    }

    if (!sensor.extract_features(attempt + 1).has_value())
    {
      state.SkipWithError("Failed to prepare test template");
      return;
    }

    if (attempt == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(test_id).has_value())
  {
    state.SkipWithError("Failed to store test template");
    return;
  }

  std::vector<double> latencies_ms;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.load_model(test_id, 2);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (!result.has_value())
    {
      state.SkipWithError("Load model failed");
      return;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];

  // Cleanup
  (void)sensor.erase_model(test_id, 1); // Suppress [[nodiscard]] warning
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_LoadModelLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(50);

/**
 * @brief Benchmark getting device settings
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_GetDeviceSettingsLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto result = sensor.query_device_settings();
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Failed to get device settings");
      return;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_GetDeviceSettingsLatency)
  ->Unit(benchmark::kMicrosecond);

/**
 * @brief Benchmark model count query
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_ModelCountLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto result = sensor.model_count();
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Failed to get model count");
      return;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_ModelCountLatency)
  ->Unit(benchmark::kMicrosecond);

//=============================================================================
// DATABASE OPERATION BENCHMARKS
//=============================================================================

/**
 * @brief Benchmark template erasure
 */
BENCHMARK_DEFINE_F(SensorFixture, BM_Hardware_EraseModelLatency)(benchmark::State& state)
{
  if (!hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  // Pre-loop setup: Pre-populate templates to erase (timer hasn't started)
  std::vector<uint16_t> test_ids;
  for (uint16_t id = 220; id < 240; ++id)
  {
    test_ids.push_back(id);
  }

  // Create test templates (simplified)
  int templates_created = 0;
  for (auto id : test_ids)
  {
    // Quick enrollment for testing
    bool success = false;
    for (int attempt = 0; attempt < 2 && !success; ++attempt)
    {
      bool captured = false;
      for (int retry = 0; retry < 20; ++retry)
      {
        if (sensor.capture_image().has_value())
        {
          captured = true;
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      if (!captured)
      {
        break;
      }

      if (sensor.extract_features(attempt + 1).has_value())
      {
        if (attempt == 0)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        else
        {
          if (sensor.merge_model().has_value() && sensor.store_model(id).has_value())
          {
            success = true;
            templates_created++;
          }
        }
      }
    }
  }

  size_t erase_index = 0;

  for (auto _ : state)
  {
    if (erase_index >= test_ids.size())
    {
      break; // Ran out of templates
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.erase_model(test_ids[erase_index], 1);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    state.counters["Latency_ms"] = latency;

    erase_index++;
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_Hardware_EraseModelLatency)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(10);

} // namespace carbio::performance_tests
