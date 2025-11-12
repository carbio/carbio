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
 * @brief Benchmark linear search (search_model) vs database size
 *
 * CRITICAL METRIC: Validates O(n) time complexity
 * MEASURES: Search latency scaling with database size
 * PREREQUISITE: Database populated with templates
 */
static void BM_Scalability_SearchModel_LinearSearch(benchmark::State& state)
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

  size_t db_size = state.range(0);

  // Verify database has required templates
  auto count = sensor.model_count();
  if (!count.has_value() || count.value() < db_size)
  {
    sensor.disconnect();
    state.SkipWithError("Database not populated - need at least " + std::to_string(db_size) + " templates");
    return;
  }

  // Pre-loop setup: Capture and extract for search testing (timer hasn't started)
  SPDLOG_INFO("Place finger ONCE (15 seconds)");

  bool ready = false;
  for (int retry = 0; retry < 150 && !ready; ++retry)
  {
    if (sensor.capture_image().has_value())
    {
      if (sensor.extract_features(1).has_value())
      {
        ready = true;
        SPDLOG_INFO("Captured");
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (!ready)
  {
    sensor.disconnect();
    state.SkipWithError("Failed to capture/extract prerequisite");
    return;
  }

  auto template_data = sensor.download_model(1);
  if (!template_data.has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to download template");
    return;
  }
  SPDLOG_INFO("Template captured - remove finger now");

  int matches = 0;

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!sensor.upload_model(template_data.value().as_span(), 1).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.search_model(0, 1, db_size);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (result.has_value())
    {
      matches++;
      state.counters["MatchConfidence"] = result->confidence.get();
    }
  }

  state.counters["DBSize"] = db_size;
  state.counters["MatchRate"] = benchmark::Counter(matches, benchmark::Counter::kIsRate);
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
    ->Complexity(benchmark::oN)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->MinTime(5.0)
    ->MinWarmUpTime(1.0);

/**
 * @brief Benchmark fast search (fast_search_model) vs database size
 *
 * CRITICAL METRIC: Compare optimized search vs linear
 * MEASURES: Performance improvement of fast search algorithm
 */
static void BM_Scalability_FastSearchModel_Optimized(benchmark::State& state)
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

  size_t db_size = state.range(0);

  auto count = sensor.model_count();
  if (!count.has_value() || count.value() < db_size)
  {
    sensor.disconnect();
    state.SkipWithError("Database not populated");
    return;
  }

  SPDLOG_INFO("Place finger ONCE (15 seconds)");

  bool ready = false;
  for (int retry = 0; retry < 150 && !ready; ++retry)
  {
    if (sensor.capture_image().has_value())
    {
      if (sensor.extract_features(1).has_value())
      {
        ready = true;
        SPDLOG_INFO("Captured");
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (!ready)
  {
    sensor.disconnect();
    state.SkipWithError("Failed to capture/extract prerequisite");
    return;
  }

  auto template_data = sensor.download_model(1);
  if (!template_data.has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to download template");
    return;
  }
  SPDLOG_INFO("Template captured - remove finger now");

  int matches = 0;

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!sensor.upload_model(template_data.value().as_span(), 1).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.fast_search_model(0, 1, db_size);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (result.has_value())
    {
      matches++;
      state.counters["MatchConfidence"] = result->confidence.get();
    }
  }

  state.counters["DBSize"] = db_size;
  state.counters["MatchRate"] = benchmark::Counter(matches, benchmark::Counter::kIsRate);
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
    ->Complexity()
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->MinTime(5.0)
    ->MinWarmUpTime(1.0);

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
static void BM_DataTransfer_TemplateDownload(benchmark::State& state)
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

  // Store a test template first
  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t test_id = 99;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    test_id = settings->capacity - 10;  // Use an ID near the end but within range
  }
  SPDLOG_INFO("Using test template ID: {} (capacity: {})", test_id, settings.has_value() ? settings->capacity : 0);
  bool template_ready = false;

  // Pre-loop setup: Create test template (timer hasn't started)
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

    if (sensor.extract_features(attempt + 1).has_value())
    {
      if (attempt == 1)
      {
        if (sensor.merge_model().has_value() && sensor.store_model(test_id).has_value())
        {
          template_ready = true;
        }
      }
      else
      {
        SPDLOG_INFO("Remove finger");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }
    }
  }

  if (!template_ready)
  {
    sensor.disconnect();
    state.SkipWithError("Failed to create test template");
    return;
  }

  if (!sensor.load_model(test_id, 1).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to load test template");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.download_model(1);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (result.has_value())
    {
      state.counters["TemplateSize_bytes"] = result->size();
    }
    else
    {
      state.SkipWithError("Download failed");
      break;
    }
  }

  (void)sensor.erase_model(test_id, 1);
  sensor.disconnect();
}
BENCHMARK(BM_DataTransfer_TemplateDownload)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

/**
 * @brief Benchmark template upload performance
 *
 * CRITICAL METRIC: Template injection bandwidth
 * MEASURES: Time to upload template to sensor memory
 */
static void BM_DataTransfer_TemplateUpload(benchmark::State& state)
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

  // Pre-loop setup: Download a valid template to use for upload testing (timer hasn't started)
  // Query sensor capacity and use a safe template ID
  auto settings = sensor.query_device_settings();
  uint16_t test_id = 98;  // Default safe ID
  if (settings.has_value() && settings->capacity > 100)
  {
    test_id = settings->capacity - 11;  // Use an ID near the end but within range
  }
  SPDLOG_INFO("Using test template ID: {} (capacity: {})", test_id, settings.has_value() ? settings->capacity : 0);

  // Create template
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

    if (!sensor.extract_features(attempt + 1).has_value())
    {
      sensor.disconnect();
      state.SkipWithError("Failed to extract features");
      return;
    }

    if (attempt == 0)
    {
      SPDLOG_INFO("Remove finger");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  if (!sensor.merge_model().has_value() || !sensor.store_model(test_id).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to create test template");
    return;
  }

  // Load and download to get template data
  if (!sensor.load_model(test_id, 1).has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to load test template");
    return;
  }

  auto template_data = sensor.download_model(1);
  if (!template_data.has_value())
  {
    sensor.disconnect();
    state.SkipWithError("Failed to download test template");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.upload_model(template_data->as_span(), 2);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Upload failed");
      break;
    }
  }

  state.counters["TemplateSize_bytes"] = template_data->size();

  (void)sensor.erase_model(test_id, 1);
  sensor.disconnect();
}
BENCHMARK(BM_DataTransfer_TemplateUpload)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

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
static void BM_Database_ReadIndexTable(benchmark::State& state)
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

  std::array<std::uint8_t, 32> buffer;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.read_index_table(buffer);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();

    if (!result.has_value())
    {
      state.SkipWithError("read_index_table failed");
      sensor.disconnect();
      return;
    }

    int template_count = 0;
    for (size_t i = 0; i < 256; ++i)
    {
      size_t byte_idx = i / 8;
      size_t bit_idx = i % 8;
      if ((result->at(byte_idx) & (1 << bit_idx)) != 0)
      {
        template_count++;
      }
    }
    state.counters["TemplatesFound"] = template_count;
  }

  sensor.disconnect();
}
BENCHMARK(BM_Database_ReadIndexTable)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

/**
 * @brief Benchmark clearing entire database
 *
 * CRITICAL METRIC: Emergency database wipe performance
 * MEASURES: Time to erase all templates
 */
static void BM_Database_ClearAllTemplates(benchmark::State& state)
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

  // Query sensor capacity to determine valid template ID range
  auto settings = sensor.query_device_settings();
  uint16_t base_id = 10; // Default safe starting ID
  if (settings.has_value())
  {
    uint16_t capacity = settings->capacity;
    // Use IDs starting from capacity - 10 (leaving room for 5 templates)
    if (capacity > 15)
    {
      base_id = capacity - 10;
    }
    SPDLOG_INFO("Sensor capacity: {}, using base template ID: {}", capacity, base_id);
  }

  // Pre-loop setup: Populate a few templates first for realistic test (timer hasn't started)
  for (int i = 0; i < 5; ++i)
  {
    for (int attempt = 0; attempt < 2; ++attempt)
    {
      SPDLOG_INFO("Place finger on sensor");

      bool captured = false;
      for (int retry = 0; retry < 30; ++retry)
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
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
    }
    (void)sensor.merge_model();                 // Suppress [[nodiscard]] warning
    (void)sensor.store_model(base_id + i);      // Suppress [[nodiscard]] warning - use valid IDs within capacity
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    auto before_count = sensor.model_count();
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.clear_database();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("clear_database failed");
      break;
    }

    if (before_count.has_value())
    {
      state.counters["TemplatesCleared"] = before_count.value();
    }
  }

  sensor.disconnect();
}
BENCHMARK(BM_Database_ClearAllTemplates)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

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
static void BM_Notepad_Write32BytePage(benchmark::State& state)
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

  std::array<std::uint8_t, 32> test_data;
  std::fill(test_data.begin(), test_data.end(), 0xAA);

  uint8_t page = 0;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.write_notepad(page, test_data);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("write_notepad failed");
      sensor.disconnect();
      return;
    }

    page = (page + 1) % 16;
  }

  sensor.disconnect();
}
BENCHMARK(BM_Notepad_Write32BytePage)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

/**
 * @brief Benchmark notepad read performance
 *
 * CRITICAL METRIC: User metadata retrieval latency
 * MEASURES: 32-byte page read time
 */
static void BM_Notepad_Read32BytePage(benchmark::State& state)
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

  uint8_t page = 0;

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = sensor.read_notepad(page);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("read_notepad failed");
      sensor.disconnect();
      return;
    }

    page = (page + 1) % 16;
  }

  sensor.disconnect();
}
BENCHMARK(BM_Notepad_Read32BytePage)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

} // namespace carbio::performance_tests
