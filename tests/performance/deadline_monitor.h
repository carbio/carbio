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

#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace carbio::performance_tests
{

/**
 * @brief Real-time performance analysis results
 */
struct RealTimeAnalysis
{
  // Deadline configuration
  uint64_t deadline_us = 0;

  // Performance counters
  uint64_t total_invocations = 0;
  uint64_t deadline_hits = 0;
  uint64_t deadline_misses = 0;
  double deadline_miss_rate_pct = 0.0;

  // Miss analysis
  std::vector<uint64_t> miss_magnitudes_us; // How much did we miss by?
  uint64_t worst_miss_us = 0;
  uint64_t consecutive_misses = 0;
  uint64_t max_consecutive_misses = 0;
  double mean_miss_magnitude_us = 0.0;

  // Timing statistics
  uint64_t min_execution_us = 0;
  uint64_t max_execution_us = 0; // Same as WCET
  double mean_execution_us = 0.0;
  double jitter_us = 0.0; // Max deviation from mean

  // Real-time classification
  enum class RTClass
  {
    HARD, // 0% deadline misses - critical safety functions
    FIRM, // < 0.1% misses - important functions with degradation tolerance
    SOFT, // < 5% misses - user comfort functions
    NONE  // >= 5% misses - not suitable for real-time
  };
  RTClass rt_class = RTClass::NONE;

  // Determinism assessment
  double jitter_ratio = 0.0;     // jitter / mean (lower is better)
  bool is_deterministic = false; // jitter < 10% of mean

  // Automotive deadline compliance (500ms HMI requirement)
  bool meets_automotive_deadline = false;

  /**
   * @brief Get human-readable real-time class string
   */
  std::string GetRTClassName() const
  {
    switch (rt_class)
    {
    case RTClass::HARD:
      return "HARD (0% misses)";
    case RTClass::FIRM:
      return "FIRM (<0.1% misses)";
    case RTClass::SOFT:
      return "SOFT (<5% misses)";
    case RTClass::NONE:
      return "NONE (not real-time)";
    default:
      return "UNKNOWN";
    }
  }
};

/**
 * @brief Real-time deadline monitoring and analysis
 *
 * Features:
 * - Configurable deadline thresholds
 * - Consecutive miss detection (critical for safety)
 * - WCET (Worst Case Execution Time) tracking
 * - Jitter analysis for determinism assessment
 * - Real-time class categorization (HARD/FIRM/SOFT/NONE)
 * - Automotive deadline compliance (500ms)
 *
 * Usage:
 * @code
 * DeadlineMonitor monitor({.deadline_us = 500000, .enforce_deadline = true});
 *
 * auto result = monitor.MonitorDeadline([&]() {
 *     return sensor.verify(user_id);
 * });
 *
 * auto analysis = monitor.GetAnalysis();
 * if (analysis.rt_class == RTClass::HARD) {
 *     // Suitable for safety-critical functions
 * }
 * @endcode
 */
class DeadlineMonitor
{
public:
  struct Config
  {
    uint64_t deadline_us;
    bool enforce_deadline;
    bool log_violations;
    int max_consecutive_misses;
    bool fail_on_any_miss;
    std::string operation_name;
  };

  explicit DeadlineMonitor(const Config& config =
                               Config{
                                   .deadline_us = 500000,        // 500ms (automotive HMI standard)
                                   .enforce_deadline = false,    // Fail test on consecutive misses
                                   .log_violations = true,       // Log each deadline miss
                                   .max_consecutive_misses = 3,  // Fail test if exceeded
                                   .fail_on_any_miss = false,    // Strict mode (for HARD real-time)
                                   .operation_name = "operation" // For logging
                               })
      : config_(config)
  {
    SPDLOG_DEBUG("DeadlineMonitor created: deadline={}us, enforce={}, max_consecutive={}", config_.deadline_us, config_.enforce_deadline, config_.max_consecutive_misses);
  }

  /**
   * @brief Monitor a single operation for deadline compliance
   *
   * @tparam Func Callable type (lambda, function pointer, std::function)
   * @param operation Operation to monitor
   * @return Result of the operation
   * @throws std::runtime_error if deadline enforcement fails
   *
   * @note Uses high_resolution_clock for precise timing (nanosecond resolution)
   * @note Automatically tracks all timing statistics
   */
  template <typename Func>
  auto MonitorDeadline(Func&& operation)
  {
    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    // Execute operation (may throw, let exception propagate)
    auto result = std::forward<Func>(operation)();

    auto end = high_resolution_clock::now();
    auto elapsed_us = duration_cast<microseconds>(end - start).count();

    // Record execution time
    RecordExecution(elapsed_us);

    // Check deadline compliance
    if (elapsed_us > static_cast<int64_t>(config_.deadline_us))
    {
      HandleDeadlineMiss(elapsed_us);
    }
    else
    {
      HandleDeadlineHit();
    }

    return result;
  }

  /**
   * @brief Get comprehensive deadline analysis
   * @return Real-time performance analysis with all metrics
   */
  RealTimeAnalysis GetAnalysis() const
  {
    RealTimeAnalysis analysis;

    // Basic configuration
    analysis.deadline_us = config_.deadline_us;
    analysis.total_invocations = total_invocations_;
    analysis.deadline_hits = deadline_hits_;
    analysis.deadline_misses = deadline_misses_;

    if (total_invocations_ > 0)
    {
      analysis.deadline_miss_rate_pct = (static_cast<double>(deadline_misses_) / total_invocations_) * 100.0;
    }

    // Miss analysis
    analysis.miss_magnitudes_us = miss_magnitudes_;
    analysis.worst_miss_us = worst_miss_us_;
    analysis.max_consecutive_misses = max_consecutive_misses_;

    if (!miss_magnitudes_.empty())
    {
      double sum = 0.0;
      for (uint64_t mag : miss_magnitudes_)
        sum += mag;
      analysis.mean_miss_magnitude_us = sum / miss_magnitudes_.size();
    }

    // Timing statistics
    if (!execution_times_us_.empty())
    {
      analysis.min_execution_us = *std::min_element(execution_times_us_.begin(), execution_times_us_.end());
      analysis.max_execution_us = *std::max_element(execution_times_us_.begin(), execution_times_us_.end());

      double sum = 0.0;
      for (uint64_t t : execution_times_us_)
        sum += t;
      analysis.mean_execution_us = sum / execution_times_us_.size();

      // Jitter = max deviation from mean
      double max_deviation = 0.0;
      for (uint64_t t : execution_times_us_)
      {
        double deviation = std::abs(static_cast<double>(t) - analysis.mean_execution_us);
        max_deviation = std::max(max_deviation, deviation);
      }
      analysis.jitter_us = max_deviation;

      // Determinism metrics
      analysis.jitter_ratio = analysis.mean_execution_us > 0.0 ? (analysis.jitter_us / analysis.mean_execution_us) : 0.0;
      analysis.is_deterministic = (analysis.jitter_ratio < 0.10); // Jitter < 10% of mean
    }

    // Real-time classification
    if (analysis.deadline_miss_rate_pct < 1e-10)
    {
      analysis.rt_class = RealTimeAnalysis::RTClass::HARD;
    }
    else if (analysis.deadline_miss_rate_pct < 0.1)
    {
      analysis.rt_class = RealTimeAnalysis::RTClass::FIRM;
    }
    else if (analysis.deadline_miss_rate_pct < 5.0)
    {
      analysis.rt_class = RealTimeAnalysis::RTClass::SOFT;
    }
    else
    {
      analysis.rt_class = RealTimeAnalysis::RTClass::NONE;
    }

    // Automotive compliance (500ms HMI deadline)
    constexpr uint64_t AUTOMOTIVE_DEADLINE_US = 500000; // 500ms
    analysis.meets_automotive_deadline = (analysis.max_execution_us <= AUTOMOTIVE_DEADLINE_US);

    return analysis;
  }

  /**
   * @brief Reset all monitoring data
   * Useful for running multiple test phases
   */
  void Reset()
  {
    total_invocations_ = 0;
    deadline_hits_ = 0;
    deadline_misses_ = 0;
    consecutive_misses_ = 0;
    max_consecutive_misses_ = 0;
    worst_miss_us_ = 0;
    miss_magnitudes_.clear();
    execution_times_us_.clear();
  }

  /**
   * @brief Print detailed analysis to log
   */
  void PrintAnalysis() const
  {
    auto analysis = GetAnalysis();

    SPDLOG_INFO("========================================");
    SPDLOG_INFO("Real-Time Deadline Analysis: {}", config_.operation_name);
    SPDLOG_INFO("========================================");
    SPDLOG_INFO("Deadline: {} us ({}ms)", analysis.deadline_us, analysis.deadline_us / 1000.0);
    SPDLOG_INFO("Total Invocations: {}", analysis.total_invocations);
    SPDLOG_INFO("Deadline Hits: {}", analysis.deadline_hits);
    SPDLOG_INFO("Deadline Misses: {} ({:.2f}%)", analysis.deadline_misses, analysis.deadline_miss_rate_pct);
    SPDLOG_INFO("");

    SPDLOG_INFO("Execution Time Statistics:");
    SPDLOG_INFO("  Min:  {} us ({:.2f} ms)", analysis.min_execution_us, analysis.min_execution_us / 1000.0);
    SPDLOG_INFO("  Mean: {} us ({:.2f} ms)", static_cast<uint64_t>(analysis.mean_execution_us), analysis.mean_execution_us / 1000.0);
    SPDLOG_INFO("  Max:  {} us ({:.2f} ms) [WCET]", analysis.max_execution_us, analysis.max_execution_us / 1000.0);
    SPDLOG_INFO("");

    SPDLOG_INFO("Determinism Metrics:");
    SPDLOG_INFO("  Jitter: {} us ({:.2f}%)", static_cast<uint64_t>(analysis.jitter_us), analysis.jitter_ratio * 100.0);
    SPDLOG_INFO("  Is Deterministic: {}", analysis.is_deterministic ? "YES" : "NO");
    SPDLOG_INFO("");

    SPDLOG_INFO("Real-Time Classification: {}", analysis.GetRTClassName());
    SPDLOG_INFO("Automotive Deadline (500ms): {}", analysis.meets_automotive_deadline ? "PASS" : "FAIL");

    if (analysis.deadline_misses > 0)
    {
      SPDLOG_WARN("");
      SPDLOG_WARN("Deadline Miss Analysis:");
      SPDLOG_WARN("  Worst Miss: {} us", analysis.worst_miss_us);
      SPDLOG_WARN("  Mean Miss Magnitude: {} us", static_cast<uint64_t>(analysis.mean_miss_magnitude_us));
      SPDLOG_WARN("  Max Consecutive Misses: {}", analysis.max_consecutive_misses);
    }

    SPDLOG_INFO("========================================");
  }

private:
  Config config_;

  // Performance counters
  uint64_t total_invocations_ = 0;
  uint64_t deadline_hits_ = 0;
  uint64_t deadline_misses_ = 0;
  uint64_t consecutive_misses_ = 0;
  uint64_t max_consecutive_misses_ = 0;
  uint64_t worst_miss_us_ = 0;

  // Timing data
  std::vector<uint64_t> miss_magnitudes_;
  std::vector<uint64_t> execution_times_us_;

  void RecordExecution(uint64_t elapsed_us)
  {
    total_invocations_++;
    execution_times_us_.push_back(elapsed_us);
  }

  void HandleDeadlineHit()
  {
    deadline_hits_++;
    consecutive_misses_ = 0;
  }

  void HandleDeadlineMiss(uint64_t elapsed_us)
  {
    deadline_misses_++;
    consecutive_misses_++;
    max_consecutive_misses_ = std::max(max_consecutive_misses_, consecutive_misses_);

    uint64_t miss_magnitude = elapsed_us - config_.deadline_us;
    miss_magnitudes_.push_back(miss_magnitude);
    worst_miss_us_ = std::max(worst_miss_us_, miss_magnitude);

    if (config_.log_violations)
    {
      SPDLOG_WARN("DEADLINE MISS #{}: {}us (deadline: {}us, miss by: {}us) [{}]", deadline_misses_, elapsed_us, config_.deadline_us, miss_magnitude, config_.operation_name);
    }

    // Check enforcement policy
    if (config_.fail_on_any_miss)
    {
      throw std::runtime_error("HARD real-time deadline violation - any miss is fatal");
    }

    if (config_.enforce_deadline && consecutive_misses_ >= static_cast<uint64_t>(config_.max_consecutive_misses))
    {
      throw std::runtime_error("Exceeded maximum consecutive deadline misses: " + std::to_string(consecutive_misses_));
    }
  }
};

} // namespace carbio::performance_tests
