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
#include <memory>
#include <thread>

namespace carbio::performance_tests
{

static std::unique_ptr<carbio::fingerprint_sensor> g_sensor;
static bool g_hardware_ready = false;
static uint16_t g_test_id_1 = 50;
static uint16_t g_test_id_2 = 51;

struct hardware_config
{
  static constexpr const char* device_path = "/dev/ttyAMA0";
  static constexpr const char* device_path_usb = "/dev/ttyUSB0";
};

bool is_hardware_available()
{
  return g_hardware_ready && g_sensor != nullptr;
}

bool open_sensor(fingerprint_sensor& sensor)
{
  if (sensor.is_connected()) return true;
  return sensor.connect(hardware_config::device_path) || sensor.connect(hardware_config::device_path_usb);
}

bool wait_for_finger(int timeout_deciseconds)
{
  for (int retry = 0; retry < timeout_deciseconds; ++retry)
  {
    if (g_sensor->capture_image().has_value()) return true;
    if (retry == 50) SPDLOG_INFO("  [10 seconds remaining...]");
    else if (retry == 100) SPDLOG_INFO("  [5 seconds remaining...]");
    else if (retry == 130) SPDLOG_INFO("  [2 seconds remaining...]");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

bool create_template(uint16_t template_id)
{
  for (int step = 0; step < 2; ++step)
  {
    SPDLOG_INFO("Step {}/2: Place finger (15 seconds)", step + 1);
    if (!wait_for_finger(150))
    {
      SPDLOG_ERROR("Timeout waiting for finger");
      return false;
    }
    SPDLOG_INFO("Image captured");
    if (!g_sensor->extract_features(step + 1).has_value())
    {
      SPDLOG_ERROR("Failed to extract features");
      return false;
    }
    if (step == 0)
    {
      SPDLOG_INFO("Remove finger and wait 2 seconds");
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  }
  if (!g_sensor->merge_model().has_value())
  {
    SPDLOG_ERROR("Failed to create model");
    return false;
  }
  if (!g_sensor->store_model(template_id).has_value())
  {
    SPDLOG_ERROR("Failed to store model at ID {}", template_id);
    return false;
  }
  SPDLOG_INFO("Template {} created", template_id);
  return true;
}

class SensorFixture : public benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State&) override
  {
    if (!g_hardware_ready || !g_sensor)
    {
      SPDLOG_ERROR("Global sensor not ready");
    }
  }
  void TearDown(const ::benchmark::State&) override {}
};

BENCHMARK_DEFINE_F(SensorFixture, BM_CaptureImage)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }

  auto captured_image = g_sensor->download_image();
  if (!captured_image.has_value())
  {
    state.SkipWithError("Failed to download fingerprint image");
    return;
  }
  SPDLOG_INFO("Fingerprint captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_image(captured_image.value().as_span()).has_value())
    {
      state.SkipWithError("Failed to upload image");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->capture_image();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Capture failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_CaptureImage)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_ExtractFeatures)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }

  auto captured_image = g_sensor->download_image();
  if (!captured_image.has_value())
  {
    state.SkipWithError("Failed to download fingerprint image");
    return;
  }
  SPDLOG_INFO("Fingerprint captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_image(captured_image.value().as_span()).has_value())
    {
      state.SkipWithError("Failed to upload image");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->extract_features(1);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Extraction failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_ExtractFeatures)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_MergeModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger TWICE (2 captures)");
  for (int step = 0; step < 2; ++step)
  {
    SPDLOG_INFO("Capture {}/2: Place finger (15 seconds)", step + 1);
    if (!wait_for_finger(150))
    {
      state.SkipWithError("Timeout waiting for finger");
      return;
    }
    if (!g_sensor->extract_features(step + 1).has_value())
    {
      state.SkipWithError("Failed to extract features");
      return;
    }
    if (step == 0)
    {
      SPDLOG_INFO("Remove finger and wait 2 seconds");
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  }

  auto template1 = g_sensor->download_model(1);
  auto template2 = g_sensor->download_model(2);
  if (!template1.has_value() || !template2.has_value())
  {
    state.SkipWithError("Failed to download templates");
    return;
  }
  SPDLOG_INFO("Templates captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_model(template1.value().as_span(), 1).has_value() ||
        !g_sensor->upload_model(template2.value().as_span(), 2).has_value())
    {
      state.SkipWithError("Failed to upload templates");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->merge_model();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Merge failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_MergeModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_StoreModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  auto settings = g_sensor->query_device_settings();
  uint16_t capacity = settings.has_value() ? settings->capacity : 150;
  uint16_t start_id = (capacity > 120) ? 100 : 10;

  if (!g_sensor->load_model(g_test_id_1, 1).has_value())
  {
    state.SkipWithError("Failed to load test template");
    return;
  }

  auto template_data = g_sensor->download_model(1);
  if (!template_data.has_value())
  {
    state.SkipWithError("Failed to download test template");
    return;
  }

  uint16_t current_id = start_id;

  for (auto _ : state)
  {
    if (current_id >= capacity)
    {
      state.SkipWithError("Insufficient capacity");
      break;
    }

    state.PauseTiming();
    if (!g_sensor->upload_model(template_data.value().as_span(), 2).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->store_model(current_id);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Store failed");
      break;
    }

    state.PauseTiming();
    g_sensor->erase_model(current_id, 1);
    current_id++;
    state.ResumeTiming();
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_StoreModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_LoadModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->load_model(g_test_id_1, 2);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Load failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_LoadModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_SearchModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  auto settings = g_sensor->query_device_settings();
  uint16_t search_count = settings.has_value() ? settings->capacity : 150;

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }

  if (!g_sensor->extract_features(1).has_value())
  {
    state.SkipWithError("Failed to extract features");
    return;
  }

  auto template_data = g_sensor->download_model(1);
  if (!template_data.has_value())
  {
    state.SkipWithError("Failed to download template");
    return;
  }
  SPDLOG_INFO("Template captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_model(template_data.value().as_span(), 1).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->search_model(0, 1, search_count);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Search failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_SearchModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_FastSearchModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  auto settings = g_sensor->query_device_settings();
  uint16_t search_count = settings.has_value() ? settings->capacity : 150;

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }

  if (!g_sensor->extract_features(1).has_value())
  {
    state.SkipWithError("Failed to extract features");
    return;
  }

  auto template_data = g_sensor->download_model(1);
  if (!template_data.has_value())
  {
    state.SkipWithError("Failed to download template");
    return;
  }
  SPDLOG_INFO("Template captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_model(template_data.value().as_span(), 1).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->fast_search_model(0, 1, search_count);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Fast search failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_FastSearchModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_MatchModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger TWICE (2 captures)");
  for (int step = 0; step < 2; ++step)
  {
    SPDLOG_INFO("Capture {}/2: Place finger (15 seconds)", step + 1);
    if (!wait_for_finger(150))
    {
      state.SkipWithError("Timeout waiting for finger");
      return;
    }
    if (!g_sensor->extract_features(step + 1).has_value())
    {
      state.SkipWithError("Failed to extract features");
      return;
    }
    if (step == 0)
    {
      SPDLOG_INFO("Remove finger and wait 2 seconds");
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  }

  auto template1 = g_sensor->download_model(1);
  auto template2 = g_sensor->download_model(2);
  if (!template1.has_value() || !template2.has_value())
  {
    state.SkipWithError("Failed to download templates");
    return;
  }
  SPDLOG_INFO("Templates captured - remove finger now");

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_model(template1.value().as_span(), 1).has_value() ||
        !g_sensor->upload_model(template2.value().as_span(), 2).has_value())
    {
      state.SkipWithError("Failed to upload templates");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->match_model();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Match failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_MatchModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_QueryDeviceSettings)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->query_device_settings();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Query failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_QueryDeviceSettings)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_ModelCount)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->model_count();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Count failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_ModelCount)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_EraseModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  auto settings = g_sensor->query_device_settings();
  uint16_t capacity = settings.has_value() ? settings->capacity : 150;
  if (capacity <= 30)
  {
    state.SkipWithError("Sensor capacity too small");
    return;
  }

  uint16_t start_id = (capacity > 50) ? 20 : 15;

  if (!g_sensor->load_model(g_test_id_1, 1).has_value())
  {
    state.SkipWithError("Failed to load test template");
    return;
  }

  auto template_data = g_sensor->download_model(1);
  if (!template_data.has_value())
  {
    state.SkipWithError("Failed to download test template");
    return;
  }

  uint16_t current_id = start_id;

  for (auto _ : state)
  {
    state.PauseTiming();
    if (!g_sensor->upload_model(template_data.value().as_span(), 2).has_value())
    {
      state.SkipWithError("Failed to upload template");
      break;
    }
    if (!g_sensor->store_model(current_id).has_value())
    {
      state.SkipWithError("Failed to create template for erasure");
      break;
    }
    state.ResumeTiming();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->erase_model(current_id, 1);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Erase failed");
      break;
    }

    current_id++;
    if (current_id >= capacity) current_id = start_id;
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_EraseModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_DownloadModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  if (!g_sensor->load_model(g_test_id_1, 1).has_value())
  {
    state.SkipWithError("Failed to load test template");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->download_model(1);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Download failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_DownloadModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_UploadModel)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  if (!g_sensor->load_model(g_test_id_1, 1).has_value())
  {
    state.SkipWithError("Failed to load test template");
    return;
  }

  auto template_data = g_sensor->download_model(1);
  if (!template_data.has_value())
  {
    state.SkipWithError("Failed to download test template");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->upload_model(template_data.value().as_span(), 2);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Upload failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_UploadModel)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_DownloadImage)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }
  SPDLOG_INFO("Fingerprint captured - remove finger now");

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->download_image();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Download image failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_DownloadImage)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_UploadImage)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  SPDLOG_INFO("Place finger ONCE (15 seconds)");
  if (!wait_for_finger(150))
  {
    state.SkipWithError("Timeout waiting for finger");
    return;
  }

  auto image = g_sensor->download_image();
  if (!image.has_value())
  {
    state.SkipWithError("Failed to download image");
    return;
  }
  SPDLOG_INFO("Fingerprint captured - remove finger now");

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->upload_image(image.value().as_span());
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Upload image failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_UploadImage)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(5.0)
  ->MinWarmUpTime(1.0);

BENCHMARK_DEFINE_F(SensorFixture, BM_ReadIndexTable)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  std::vector<uint8_t> buffer(32);

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->read_index_table(buffer);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();

    if (!result.has_value())
    {
      state.SkipWithError("Read index table failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_ReadIndexTable)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_WriteNotepad)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  std::array<uint8_t, 32> data;
  data.fill(0xAB);

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->write_notepad(0, data);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Write notepad failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_WriteNotepad)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_ReadNotepad)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->read_notepad(0);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Read notepad failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_ReadNotepad)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_TurnLedOn)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->turn_led_on();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Turn LED on failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_TurnLedOn)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_TurnLedOff)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->turn_led_off();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Turn LED off failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_TurnLedOff)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

BENCHMARK_DEFINE_F(SensorFixture, BM_SetLedSetting)(benchmark::State& state)
{
  if (!g_hardware_ready)
  {
    state.SkipWithError("R30x sensor not available");
    return;
  }

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = g_sensor->set_led_setting(led_mode_setting::breathing, 128, led_color_setting::blue, 0);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration<double>(end - start).count());
    benchmark::DoNotOptimize(result);

    if (!result.has_value())
    {
      state.SkipWithError("Set LED setting failed");
      break;
    }
  }
}
BENCHMARK_REGISTER_F(SensorFixture, BM_SetLedSetting)
  ->Unit(benchmark::kMicrosecond)
  ->UseManualTime()
  ->MinTime(2.0)
  ->MinWarmUpTime(0.5);

void initialize_hardware_benchmarks()
{
  SPDLOG_INFO("Hardware Benchmark Suite - R30x Fingerprint Sensor");
  SPDLOG_INFO("Initializing global sensor");

  g_sensor = std::make_unique<carbio::fingerprint_sensor>();

  if (!open_sensor(*g_sensor))
  {
    SPDLOG_ERROR("Failed to connect to R30x sensor");
    SPDLOG_ERROR("Check sensor is connected to /dev/ttyAMA0 or /dev/ttyUSB0");
    g_hardware_ready = false;
    g_sensor.reset();
    return;
  }

  SPDLOG_INFO("Sensor connected");
  g_hardware_ready = true;
  spdlog::set_level(spdlog::level::info);

  auto settings = g_sensor->query_device_settings();
  if (settings.has_value())
  {
    uint16_t capacity = settings->capacity;
    SPDLOG_INFO("Sensor capacity: {} templates", capacity);

    if (capacity > 100) { g_test_id_1 = capacity / 2; g_test_id_2 = g_test_id_1 + 1; }
    else if (capacity > 20) { g_test_id_1 = 10; g_test_id_2 = 11; }
    else
    {
      SPDLOG_ERROR("Sensor capacity too small: {} - need at least 20", capacity);
      g_hardware_ready = false;
      g_sensor.reset();
      return;
    }
  }

  SPDLOG_INFO("Checking for existing test templates at IDs {} and {}", g_test_id_1, g_test_id_2);

  if (g_sensor->load_model(g_test_id_1, 1).has_value())
  {
    SPDLOG_INFO("Template {} already exists", g_test_id_1);
  }
  else
  {
    SPDLOG_INFO("Creating test template {}", g_test_id_1);
    if (!create_template(g_test_id_1))
    {
      SPDLOG_WARN("Failed to create template {} - some benchmarks may fail", g_test_id_1);
    }
  }

  if (g_sensor->load_model(g_test_id_2, 1).has_value())
  {
    SPDLOG_INFO("Template {} already exists", g_test_id_2);
  }
  else
  {
    SPDLOG_INFO("Creating test template {}", g_test_id_2);
    if (!create_template(g_test_id_2))
    {
      SPDLOG_WARN("Failed to create template {} - some benchmarks may fail", g_test_id_2);
    }
  }

  SPDLOG_INFO("Test templates ready");
  SPDLOG_INFO("Running benchmarks");
  SPDLOG_INFO("");
  SPDLOG_INFO("NOTE: This benchmark suite is NOT THREAD-SAFE");
  SPDLOG_INFO("Do not use --benchmark_threads option");
}

void cleanup_hardware_benchmarks()
{
  if (!g_sensor || !g_hardware_ready) return;

  SPDLOG_INFO("Cleaning up test templates");

  g_sensor->erase_model(g_test_id_1, 1);
  g_sensor->erase_model(g_test_id_2, 1);

  for (uint16_t id = 15; id < 40; ++id) g_sensor->erase_model(id, 1);
  for (uint16_t id = 100; id < 110; ++id) g_sensor->erase_model(id, 1);

  SPDLOG_INFO("Disconnecting sensor");
  g_sensor->disconnect();
  g_sensor.reset();
  g_hardware_ready = false;

  SPDLOG_INFO("Benchmark suite complete");
}

} // namespace carbio::performance_tests
