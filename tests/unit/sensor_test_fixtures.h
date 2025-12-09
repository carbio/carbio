/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *********************************************************************/

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "command_executor_mock.h"
#include "fingerprint/command_traits.h"
#include "fingerprint/match_query_info.h"
#include "fingerprint/result.h"
#include "fingerprint/search_query_info.h"
#include "fingerprint/sensor_types.h"

#include <chrono>
#include <memory>
#include <random>

namespace carbio::unit_tests
{

using namespace testing;

/**
 * @brief Realistic latencies for fingerprint sensor operations
 *
 * NOTE: These are NOT used for performance benchmarking.
 * They're used to make unit tests more realistic by simulating
 * the temporal behavior of the R30x sensor.
 */
struct sensor_latencies
{
  std::chrono::microseconds capture_image{250'000};     // 250ms typical
  std::chrono::microseconds extract_features{150'000};  // 150ms typical
  std::chrono::microseconds merge_model{100'000};      // 100ms typical
  std::chrono::microseconds store_model{50'000};        // 50ms typical
  std::chrono::microseconds load_model{50'000};         // 50ms typical
  std::chrono::microseconds match_model{80'000};        // 80ms typical
  std::chrono::microseconds search_per_template{5'000}; // 5ms per template
  std::chrono::microseconds erase_model{10'000};        // 10ms typical
  std::chrono::microseconds clear_database{200'000};    // 200ms typical
  std::chrono::microseconds count_model{20'000};        // 20ms typical
};

/**
 * @brief Realistic success/failure rates for fingerprint operations
 */
struct success_rates
{
  double capture_success = 0.85; // 85% first-try capture success
  double extract_success = 0.95; // 95% extraction success
  double match_success = 0.92;   // 92% match success for same finger
  double search_hit_rate = 0.90; // 90% search hit rate
};

/**
 * @brief Deterministic random number generator for reproducible tests
 */
class test_rng
{
  std::mt19937 rng_;
  std::uniform_real_distribution<double> dist_{0.0, 1.0};

public:
  explicit test_rng(unsigned seed = 12345)
      : rng_(seed)
  {
  }

  double next()
  {
    return dist_(rng_);
  }
  bool success(double probability)
  {
    return next() < probability;
  }
};

/**
 * @brief Simulates realistic fingerprint sensor behavior with configurable latencies
 *
 * USE CASE: Unit tests that need realistic sensor behavior without hardware
 * NOT FOR: Performance benchmarking (use real hardware instead)
 *
 * IMPORTANT: This class uses GMock which requires proper initialization.
 * Call initialize() before using executor() to avoid static initialization order issues.
 */
class realistic_sensor_mock
{
  std::unique_ptr<command_executor_mock> executor_;
  sensor_latencies latencies_;
  success_rates rates_;
  test_rng rng_;
  bool initialized_ = false;

  template <typename Duration>
  void simulate_latency(Duration d)
  {
    // For unit tests, we can actually sleep to test timeout handling
    // For faster tests, this can be disabled
    // std::this_thread::sleep_for(d);

    // Or just ensure compiler doesn't optimize away
    volatile auto count = d.count();
    (void)count;
  }

public:
  explicit realistic_sensor_mock(sensor_latencies latencies = {}, success_rates rates = {}, unsigned seed = 12345)
      : latencies_(latencies)
      , rates_(rates)
      , rng_(seed)
  {
    // DO NOT call setup_expectations() here!
    // This would cause static initialization order fiasco with GMock's global registry
  }

  ~realistic_sensor_mock()
  {
    // Explicitly verify and clear expectations to avoid GMock leak warnings
    if (initialized_ && executor_)
    {
      testing::Mock::VerifyAndClearExpectations(executor_.get());
    }
  }

  // Disable copy/move
  realistic_sensor_mock(const realistic_sensor_mock&) = delete;
  realistic_sensor_mock& operator=(const realistic_sensor_mock&) = delete;
  realistic_sensor_mock(realistic_sensor_mock&&) = delete;
  realistic_sensor_mock& operator=(realistic_sensor_mock&&) = delete;

  /**
   * @brief Initialize the mock executor and setup expectations
   * MUST be called before using executor() - typically in SetUp()
   */
  void initialize()
  {
    if (initialized_)
      return;
    executor_ = std::make_unique<command_executor_mock>();
    setup_expectations();
    initialized_ = true;
  }

  command_executor_mock& executor()
  {
    if (!initialized_)
    {
      initialize();
    }
    return *executor_;
  }

  void setup_expectations()
  {
    // Capture image - may fail based on success rate
    ON_CALL(*executor_, execute_capture_image(_))
        .WillByDefault(
            [this](auto&&) -> void_result
            {
              simulate_latency(latencies_.capture_image);
              if (rng_.success(rates_.capture_success))
              {
                return make_success();
              }
              return make_error(status_code::no_finger);
            });

    // Extract features - high success rate
    ON_CALL(*executor_, execute_extract_features(_))
        .WillByDefault(
            [this](auto&&) -> void_result
            {
              simulate_latency(latencies_.extract_features);
              if (rng_.success(rates_.extract_success))
              {
                return make_success();
              }
              return make_error(status_code::image_too_faint);
            });

    // Create model - always succeeds if buffers are valid
    ON_CALL(*executor_, execute_merge_model(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.merge_model);
              return make_success();
            });

    // Store model - always succeeds
    ON_CALL(*executor_, execute_store_model(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.store_model);
              return make_success();
            });

    // Load model - always succeeds
    ON_CALL(*executor_, execute_load_model(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.load_model);
              return make_success();
            });

    // Match model - based on match success rate
    ON_CALL(*executor_, execute_match_model(_))
        .WillByDefault(
            [this](auto&&) -> result<match_query_info>
            {
              simulate_latency(latencies_.match_model);
              if (rng_.success(rates_.match_success))
              {
                match_query_info info;
                info.confidence = static_cast<uint16_t>(150 + rng_.next() * 100); // 150-250
                return make_success(info);
              }
              return make_error(status_code::no_match);
            });

    // Search model - linear time complexity
    ON_CALL(*executor_, execute_search_model(_))
        .WillByDefault(
            [this](const auto& req) -> result<search_query_info>
            {
              auto search_time = latencies_.search_per_template * req.count;
              simulate_latency(search_time);

              if (rng_.success(rates_.search_hit_rate))
              {
                search_query_info info;
                info.index = static_cast<uint16_t>(rng_.next() * req.count);
                info.confidence = static_cast<uint16_t>(150 + rng_.next() * 100);
                return make_success(info);
              }
              return make_error(status_code::not_found);
            });

    // Erase model
    ON_CALL(*executor_, execute_erase_model(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.erase_model);
              return make_success();
            });

    // Clear database
    ON_CALL(*executor_, execute_clear_database(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.clear_database);
              return make_success();
            });

    // Count models
    ON_CALL(*executor_, execute_count_model(_))
        .WillByDefault(
            [this](auto&&)
            {
              simulate_latency(latencies_.count_model);
              return make_success<uint16_t>(100);
            });
  }

  // Enable expectations for specific commands
  void expect_capture_image(int times = 1)
  {
    if (!initialized_)
      initialize();
    EXPECT_CALL(*executor_, execute_capture_image(_)).Times(AtLeast(times));
  }

  void expect_extract_features(int times = 1)
  {
    if (!initialized_)
      initialize();
    EXPECT_CALL(*executor_, execute_extract_features(_)).Times(AtLeast(times));
  }

  void expect_merge_model(int times = 1)
  {
    if (!initialized_)
      initialize();
    EXPECT_CALL(*executor_, execute_merge_model(_)).Times(AtLeast(times));
  }

  void expect_store_model(int times = 1)
  {
    if (!initialized_)
      initialize();
    EXPECT_CALL(*executor_, execute_store_model(_)).Times(AtLeast(times));
  }

  void expect_search_model(int times = 1)
  {
    if (!initialized_)
      initialize();
    EXPECT_CALL(*executor_, execute_search_model(_)).Times(AtLeast(times));
  }
};

/**
 * @brief Base test fixture with realistic sensor mock
 *
 * Use this for unit tests that need to test business logic
 * that interacts with the fingerprint sensor.
 */
class RealisticSensorTest : public ::testing::Test
{
protected:
  realistic_sensor_mock sensor_;

  RealisticSensorTest()
      : sensor_(sensor_latencies{}, success_rates{}, 12345)
  {
  }

  void SetUp() override
  {
    sensor_.initialize();
  }

  void TearDown() override
  {
    // Verify all expectations were met
    testing::Mock::VerifyAndClearExpectations(&sensor_.executor());
  }
};

} // namespace carbio::unit_tests
