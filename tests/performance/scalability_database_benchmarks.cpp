/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * DATABASE SCALABILITY & OPERATIONS BENCHMARKS
 *
 * Tests performance characteristics of database operations:
 *
 * 1. Search Scalability: O(n) validation for search_model vs fast_search_model
 * 2. Template Transfer: Upload/download bandwidth measurements
 * 3. Bulk Operations: Database clear, index table reads
 * 4. Notepad I/O: Metadata storage performance (32-byte pages)
 *********************************************************************/

#include "fingerprint/fingerprint_sensor.h"
#include "test_utilities.h"

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

namespace carbio::performance_tests {

//=============================================================================
// SEARCH SCALABILITY BENCHMARKS
//
// The most important metric for a biometric system is search performance.
// - search_model(): Linear search O(n) - tests every template sequentially
// - fast_search_model(): Optimized search algorithm
//
// These benchmarks validate algorithmic complexity and measure real performance.
//=============================================================================

/**
 * @brief Benchmark linear search (search_model) vs database size
 *
 * CRITICAL METRIC: Validates O(n) time complexity
 * MEASURES: Search latency scaling with database size
 * PREREQUISITE: Database populated with templates
 */
static void BM_Scalability_SearchModel_LinearSearch(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  size_t db_size = state.range(0);

  // Verify database has required templates
  auto count = sensor.model_count();
  if (!count.has_value() || count.value() < db_size) {
    sensor.disconnect();
    state.SkipWithError("Database not populated - need at least " +
                        std::to_string(db_size) + " templates");
    return;
  }

  // Pre-loop setup: Capture and extract for search testing (timer hasn't started)
  spdlog::info("Place finger on sensor");

  bool ready = false;
  for (int retry = 0; retry < 100 && !ready; ++retry) {
    if (sensor.capture_image().has_value()) {
      if (sensor.extract_features(1).has_value()) {
        ready = true;
        spdlog::info("Captured");
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!ready) {
    sensor.disconnect();
    state.SkipWithError("Failed to capture/extract prerequisite");
    return;
  }

  std::vector<double> latencies_ms;
  int searches = 0;
  int matches = 0;

  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.search_model(0, 1, db_size);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);
    searches++;

    if (result.has_value()) {
      matches++;
      state.counters["MatchConfidence"] = result->confidence.get();
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  // Report critical metrics
  state.counters["DBSize"] = db_size;
  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Latency_P99_ms"] = latencies_ms[n * 99 / 100];
  state.counters["MatchRate"] = static_cast<double>(matches) / searches;
  state.counters["SearchesPerSecond"] = benchmark::Counter(
    searches, benchmark::Counter::kIsRate
  );

  // For complexity analysis
  state.SetComplexityN(db_size);

  sensor.disconnect();
}
BENCHMARK(BM_Scalability_SearchModel_LinearSearch)
  ->Args({10})
  ->Args({50})
  ->Args({100})
  ->Args({200})
  ->Args({500})
  ->Args({1000})
  ->Complexity(benchmark::oN)  // Validate linear time
  ->Unit(benchmark::kMillisecond);

/**
 * @brief Benchmark fast search (fast_search_model) vs database size
 *
 * CRITICAL METRIC: Compare optimized search vs linear
 * MEASURES: Performance improvement of fast search algorithm
 */
static void BM_Scalability_FastSearchModel_Optimized(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  size_t db_size = state.range(0);

  auto count = sensor.model_count();
  if (!count.has_value() || count.value() < db_size) {
    sensor.disconnect();
    state.SkipWithError("Database not populated");
    return;
  }

  // Pre-loop setup: Capture fingerprint for testing (timer hasn't started)
  spdlog::info("Place finger on sensor");

  bool ready = false;
  for (int retry = 0; retry < 100 && !ready; ++retry) {
    if (sensor.capture_image().has_value()) {
      if (sensor.extract_features(1).has_value()) {
        ready = true;
        spdlog::info("Captured");
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!ready) {
    sensor.disconnect();
    state.SkipWithError("Failed to capture/extract prerequisite");
    return;
  }

  std::vector<double> latencies_ms;
  int searches = 0;
  int matches = 0;

  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.fast_search_model(0, 1, db_size);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);
    searches++;

    if (result.has_value()) {
      matches++;
      state.counters["MatchConfidence"] = result->confidence.get();
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["DBSize"] = db_size;
  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Latency_P99_ms"] = latencies_ms[n * 99 / 100];
  state.counters["MatchRate"] = static_cast<double>(matches) / searches;
  state.counters["SearchesPerSecond"] = benchmark::Counter(
    searches, benchmark::Counter::kIsRate
  );

  state.SetComplexityN(db_size);

  sensor.disconnect();
}
BENCHMARK(BM_Scalability_FastSearchModel_Optimized)
  ->Args({10})
  ->Args({50})
  ->Args({100})
  ->Args({200})
  ->Args({500})
  ->Args({1000})
  ->Complexity()  // Auto-detect complexity
  ->Unit(benchmark::kMillisecond);

//=============================================================================
// TEMPLATE DATA TRANSFER BENCHMARKS
//
// Template upload/download bandwidth for backup/restore operations.
// Measures: Transfer latency, bandwidth (KB/s), template size
//=============================================================================

/**
 * @brief Benchmark template download performance
 *
 * CRITICAL METRIC: Template extraction bandwidth
 * MEASURES: Time to download template from sensor memory
 */
static void BM_DataTransfer_TemplateDownload(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  // Store a test template first
  uint16_t test_id = 199;
  bool template_ready = false;

  // Pre-loop setup: Create test template (timer hasn't started)
  for (int attempt = 0; attempt < 2; ++attempt) {
    spdlog::info("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry) {
      if (sensor.capture_image().has_value()) {
        captured = true;
        spdlog::info("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured) {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    if (sensor.extract_features(attempt + 1).has_value()) {
      if (attempt == 1) {
        if (sensor.merge_model().has_value() && sensor.store_model(test_id).has_value()) {
          template_ready = true;
        }
      } else {
        spdlog::info("Remove finger");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }
    }
  }

  if (!template_ready) {
    sensor.disconnect();
    state.SkipWithError("Failed to create test template");
    return;
  }

  // Load to buffer first
  if (!sensor.load_model(test_id, 1).has_value()) {
    sensor.disconnect();
    state.SkipWithError("Failed to load test template");
    return;
  }

  std::vector<double> latencies_ms;
  size_t total_bytes = 0;

  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.download_model(1);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (result.has_value()) {
      total_bytes += result->size();
      state.counters["TemplateSize_bytes"] = result->size();
    } else {
      state.SkipWithError("Download failed");
      break;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Bandwidth_KB_per_sec"] =
    (total_bytes / 1024.0) / (std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) / 1000.0);

  (void)sensor.erase_model(test_id, 1);  // Suppress [[nodiscard]] warning
  sensor.disconnect();
}
BENCHMARK(BM_DataTransfer_TemplateDownload)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(20);

/**
 * @brief Benchmark template upload performance
 *
 * CRITICAL METRIC: Template injection bandwidth
 * MEASURES: Time to upload template to sensor memory
 */
static void BM_DataTransfer_TemplateUpload(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  // Pre-loop setup: Download a valid template to use for upload testing (timer hasn't started)
  uint16_t test_id = 198;

  // Create template
  for (int attempt = 0; attempt < 2; ++attempt) {
    spdlog::info("Place finger on sensor");

    bool captured = false;
    for (int retry = 0; retry < 50; ++retry) {
      if (sensor.capture_image().has_value()) {
        captured = true;
        spdlog::info("Captured");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!captured) {
      sensor.disconnect();
      state.SkipWithError("Timeout waiting for fingerprint");
      return;
    }

    if (!sensor.extract_features(attempt + 1).has_value()) {
      sensor.disconnect();
      state.SkipWithError("Failed to extract features");
      return;
    }

    if (attempt == 0) {
      spdlog::info("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(test_id).has_value()) {
    sensor.disconnect();
    state.SkipWithError("Failed to create test template");
    return;
  }

  // Load and download to get template data
  if (!sensor.load_model(test_id, 1).has_value()) {
    sensor.disconnect();
    state.SkipWithError("Failed to load test template");
    return;
  }

  auto template_data = sensor.download_model(1);
  if (!template_data.has_value()) {
    sensor.disconnect();
    state.SkipWithError("Failed to download test template");
    return;
  }

  std::vector<double> latencies_ms;

  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.upload_model(template_data->as_span(), 2);
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    latencies_ms.push_back(latency);

    if (!result.has_value()) {
      state.SkipWithError("Upload failed");
      break;
    }
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  size_t n = latencies_ms.size();

  state.counters["TemplateSize_bytes"] = template_data->size();
  state.counters["Latency_P50_ms"] = latencies_ms[n / 2];
  state.counters["Latency_P95_ms"] = latencies_ms[n * 95 / 100];
  state.counters["Bandwidth_KB_per_sec"] =
    (template_data->size() / 1024.0) / (std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) / n / 1000.0);

  (void)sensor.erase_model(test_id, 1);  // Suppress [[nodiscard]] warning
  sensor.disconnect();
}
BENCHMARK(BM_DataTransfer_TemplateUpload)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(20);

//=============================================================================
// DATABASE BULK OPERATIONS
//
// Large-scale database operations: clearing, indexing, bulk erasure
//=============================================================================

/**
 * @brief Benchmark reading index table (template bitmap)
 *
 * CRITICAL METRIC: How fast can we query which templates exist?
 * MEASURES: Database index query performance
 */
static void BM_Database_ReadIndexTable(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  std::array<std::uint8_t, 32> buffer;

  for (auto _ : state) {
    auto result = sensor.read_index_table(buffer);
    benchmark::DoNotOptimize(result);

    if (!result.has_value()) {
      state.SkipWithError("read_index_table failed");
      sensor.disconnect();
      return;
    }

    // Count templates
    int template_count = 0;
    for (size_t i = 0; i < 256; ++i) {
      size_t byte_idx = i / 8;
      size_t bit_idx = i % 8;
      if ((result->at(byte_idx) & (1 << bit_idx)) != 0) {
        template_count++;
      }
    }
    state.counters["TemplatesFound"] = template_count;
  }

  sensor.disconnect();
}
BENCHMARK(BM_Database_ReadIndexTable)
  ->Unit(benchmark::kMicrosecond);

/**
 * @brief Benchmark clearing entire database
 *
 * CRITICAL METRIC: Emergency database wipe performance
 * MEASURES: Time to erase all templates
 */
static void BM_Database_ClearAllTemplates(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  // Pre-loop setup: Populate a few templates first for realistic test (timer hasn't started)
  for (int i = 0; i < 5; ++i) {
    for (int attempt = 0; attempt < 2; ++attempt) {
      spdlog::info("Place finger on sensor");

      bool captured = false;
      for (int retry = 0; retry < 30; ++retry) {
        if (sensor.capture_image().has_value()) {
          captured = true;
          spdlog::info("Captured");
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      if (!captured) {
        sensor.disconnect();
        state.SkipWithError("Timeout waiting for fingerprint");
        return;
      }

      (void)sensor.extract_features(attempt + 1);  // Suppress [[nodiscard]] warning

      if (attempt == 0) {
        spdlog::info("Remove finger");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
    }
    (void)sensor.merge_model();  // Suppress [[nodiscard]] warning
    (void)sensor.store_model(150 + i);  // Suppress [[nodiscard]] warning
  }

  for (auto _ : state) {
    auto before_count = sensor.model_count();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.clear_database();
    auto end = std::chrono::high_resolution_clock::now();

    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    state.counters["ClearLatency_ms"] = latency;

    if (!result.has_value()) {
      state.SkipWithError("clear_database failed");
      break;
    }

    {
      benchmark_pause_guard pause(state);
      auto after_count = sensor.model_count();
      if (before_count.has_value()) {
        state.counters["TemplatesCleared"] = before_count.value();
      }
    }
  }

  sensor.disconnect();
}
BENCHMARK(BM_Database_ClearAllTemplates)
  ->Unit(benchmark::kMillisecond)
  ->Iterations(3);

//=============================================================================
// NOTEPAD OPERATIONS (User Metadata Storage)
//
// R30x provides 16 pages × 32 bytes for user metadata storage.
// Measures: Read/write latency for metadata operations
//=============================================================================

/**
 * @brief Benchmark notepad write performance
 *
 * CRITICAL METRIC: User metadata storage latency
 * MEASURES: 32-byte page write time
 */
static void BM_Notepad_Write32BytePage(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  std::array<std::uint8_t, 32> test_data;
  std::fill(test_data.begin(), test_data.end(), 0xAA);

  uint8_t page = 0;

  for (auto _ : state) {
    auto result = sensor.write_notepad(page, test_data);
    benchmark::DoNotOptimize(result);

    if (!result.has_value()) {
      state.SkipWithError("write_notepad failed");
      sensor.disconnect();
      return;
    }

    page = (page + 1) % 16;  // Cycle through pages 0-15
  }

  sensor.disconnect();
}
BENCHMARK(BM_Notepad_Write32BytePage)
  ->Unit(benchmark::kMicrosecond);

/**
 * @brief Benchmark notepad read performance
 *
 * CRITICAL METRIC: User metadata retrieval latency
 * MEASURES: 32-byte page read time
 */
static void BM_Notepad_Read32BytePage(benchmark::State& state) {
  if (!is_hardware_available()) {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  fingerprint_sensor sensor;
  if (!open_sensor(sensor)) {
    state.SkipWithError("Failed to open sensor");
    return;
  }

  uint8_t page = 0;

  for (auto _ : state) {
    auto result = sensor.read_notepad(page);
    benchmark::DoNotOptimize(result);

    if (!result.has_value()) {
      state.SkipWithError("read_notepad failed");
      sensor.disconnect();
      return;
    }

    page = (page + 1) % 16;  // Cycle through pages 0-15
  }

  sensor.disconnect();
}
BENCHMARK(BM_Notepad_Read32BytePage)
  ->Unit(benchmark::kMicrosecond);

} // namespace carbio::performance_tests
