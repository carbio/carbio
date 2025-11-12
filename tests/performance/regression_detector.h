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

#include "statistical_validator.h"
#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace carbio::performance_tests
{

/**
 * @brief Baseline performance data for a single benchmark
 */
struct BenchmarkBaseline
{
  std::string benchmark_name;
  std::string git_commit;
  std::string timestamp;

  // Statistical summary
  double mean = 0.0;
  double median = 0.0;
  double std_dev = 0.0;
  double p95 = 0.0;
  double p99 = 0.0;
  double ci_lower = 0.0;
  double ci_upper = 0.0;

  // Raw data (optional, for re-analysis)
  std::vector<double> raw_measurements;
};

/**
 * @brief Performance regression alert
 */
struct RegressionAlert
{
  std::string benchmark_name;
  std::string metric_name;

  double baseline_value = 0.0;
  double current_value = 0.0;
  double change_pct = 0.0;
  double p_value = 0.0;

  bool is_regression = false;
  std::string severity; // "critical", "warning", "info"

  std::string GetSeverityColor() const
  {
    if (severity == "critical")
      return "\033[0;31m"; // Red
    if (severity == "warning")
      return "\033[1;33m"; // Yellow
    return "\033[0;34m";   // Blue
  }
};

/**
 * @brief Regression detector for automated CI/CD integration
 *
 * Features:
 * - Baseline management (save/load)
 * - Statistical comparison using Mann-Whitney U
 * - Configurable regression thresholds
 * - Severity classification
 * - Alert generation
 *
 * Usage in CI/CD:
 * @code
 * RegressionDetector detector;
 * detector.LoadBaseline("baseline.txt");
 *
 * // After running benchmarks
 * std::map<std::string, std::vector<double>> current_results;
 * current_results["verify_latency"] = latencies_ms;
 *
 * auto alerts = detector.DetectRegressions(current_results);
 * if (!alerts.empty()) {
 *     detector.PrintAlerts(alerts);
 *     return 1; // Fail CI/CD pipeline
 * }
 * @endcode
 */
class RegressionDetector
{
public:
  struct Config
  {
    double critical_threshold_pct = 20.0;  // >20% change = critical
    double warning_threshold_pct = 10.0;   // >10% change = warning
    double regression_threshold_pct = 5.0; // >5% change = regression
    double significance_alpha = 0.05;      // p-value threshold
    bool fail_on_warning = false;          // Fail CI on warnings
    bool fail_on_critical = true;          // Fail CI on critical
  };

  explicit RegressionDetector(const Config& config = Config{})
      : config_(config)
  {
  }

  /**
   * @brief Save baseline to file
   */
  bool SaveBaseline(const std::map<std::string, BenchmarkBaseline>& baselines, const std::string& filename) const
  {
    std::ofstream file(filename);
    if (!file.is_open())
    {
      SPDLOG_ERROR("Failed to open baseline file for writing: {}", filename);
      return false;
    }

    // Simple text format (could be JSON in production)
    file << "# Performance Baseline\n";
    file << "# Format: benchmark_name mean median std_dev p95 p99 ci_lower ci_upper\n";
    file << "#\n";

    for (const auto& [name, baseline] : baselines)
    {
      file << name << " ";
      file << baseline.mean << " ";
      file << baseline.median << " ";
      file << baseline.std_dev << " ";
      file << baseline.p95 << " ";
      file << baseline.p99 << " ";
      file << baseline.ci_lower << " ";
      file << baseline.ci_upper << "\n";
    }

    file.close();
    SPDLOG_INFO("Baseline saved to: {}", filename);
    return true;
  }

  /**
   * @brief Load baseline from file
   */
  bool LoadBaseline(const std::string& filename)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      SPDLOG_WARN("Baseline file not found: {} (creating new baseline)", filename);
      return false;
    }

    baselines_.clear();

    std::string line;
    while (std::getline(file, line))
    {
      // Skip comments and empty lines
      if (line.empty() || line[0] == '#')
        continue;

      std::istringstream iss(line);
      BenchmarkBaseline baseline;

      if (!(iss >> baseline.benchmark_name >> baseline.mean >> baseline.median >> baseline.std_dev >> baseline.p95 >> baseline.p99 >> baseline.ci_lower >> baseline.ci_upper))
      {
        SPDLOG_WARN("Failed to parse baseline line: {}", line);
        continue;
      }

      baselines_[baseline.benchmark_name] = baseline;
    }

    file.close();
    SPDLOG_INFO("Loaded {} baseline benchmarks from: {}", baselines_.size(), filename);
    return true;
  }

  /**
   * @brief Create baseline from current results
   */
  void CreateBaseline(const std::map<std::string, std::vector<double>>& current_results)
  {
    baselines_.clear();
    StatisticalValidator validator;

    for (const auto& [name, measurements] : current_results)
    {
      if (measurements.empty())
        continue;

      auto stats = validator.Analyze(measurements);

      BenchmarkBaseline baseline;
      baseline.benchmark_name = name;
      baseline.mean = stats.mean;
      baseline.median = stats.median;
      baseline.std_dev = stats.std_dev;
      baseline.p95 = stats.p95;
      baseline.p99 = stats.p99;
      baseline.ci_lower = stats.ci_lower;
      baseline.ci_upper = stats.ci_upper;
      baseline.raw_measurements = measurements;

      baselines_[name] = baseline;
    }

    SPDLOG_INFO("Created baseline with {} benchmarks", baselines_.size());
  }

  /**
   * @brief Detect regressions by comparing current results to baseline
   */
  std::vector<RegressionAlert> DetectRegressions(const std::map<std::string, std::vector<double>>& current_results) const
  {
    std::vector<RegressionAlert> alerts;
    StatisticalValidator validator;

    for (const auto& [name, current_measurements] : current_results)
    {
      // Skip if no baseline for this benchmark
      if (baselines_.find(name) == baselines_.end())
      {
        SPDLOG_DEBUG("No baseline found for: {}", name);
        continue;
      }

      if (current_measurements.empty())
      {
        SPDLOG_WARN("No measurements for: {}", name);
        continue;
      }

      const auto& baseline = baselines_.at(name);
      auto current_stats = validator.Analyze(current_measurements);

      // Statistical comparison (simplified - could use Mann-Whitney U)
      bool is_different = false;
      if (!baseline.raw_measurements.empty())
      {
        is_different = validator.IsSignificantlyDifferent(baseline.raw_measurements, current_measurements, config_.significance_alpha);
      }
      else
      {
        // Use confidence interval method if raw data not available
        is_different = (current_stats.mean < baseline.ci_lower) || (current_stats.mean > baseline.ci_upper);
      }

      if (!is_different)
        continue;

      // Calculate change percentage
      double change_pct = ((current_stats.mean - baseline.mean) / baseline.mean) * 100.0;

      // Determine severity
      std::string severity = "info";
      bool is_regression = false;

      // For latency/time metrics: increase is bad
      // For throughput metrics: decrease is bad
      bool is_latency_metric = (name.find("latency") != std::string::npos) || (name.find("time") != std::string::npos) || (name.find("Time") != std::string::npos);

      if (is_latency_metric)
      {
        // Increase in latency is bad
        if (change_pct > config_.critical_threshold_pct)
        {
          severity = "critical";
          is_regression = true;
        }
        else if (change_pct > config_.warning_threshold_pct)
        {
          severity = "warning";
          is_regression = true;
        }
        else if (change_pct > config_.regression_threshold_pct)
        {
          is_regression = true;
        }
      }
      else
      {
        // For throughput: decrease is bad
        if (change_pct < -config_.critical_threshold_pct)
        {
          severity = "critical";
          is_regression = true;
        }
        else if (change_pct < -config_.warning_threshold_pct)
        {
          severity = "warning";
          is_regression = true;
        }
        else if (change_pct < -config_.regression_threshold_pct)
        {
          is_regression = true;
        }
      }

      if (is_regression)
      {
        RegressionAlert alert;
        alert.benchmark_name = name;
        alert.metric_name = "mean";
        alert.baseline_value = baseline.mean;
        alert.current_value = current_stats.mean;
        alert.change_pct = change_pct;
        alert.p_value = config_.significance_alpha; // Simplified
        alert.is_regression = true;
        alert.severity = severity;

        alerts.push_back(alert);
      }
    }

    return alerts;
  }

  /**
   * @brief Print regression alerts to console
   */
  void PrintAlerts(const std::vector<RegressionAlert>& alerts) const
  {
    if (alerts.empty())
    {
      SPDLOG_INFO("✓ No performance regressions detected");
      return;
    }

    SPDLOG_WARN("========================================");
    SPDLOG_WARN("PERFORMANCE REGRESSION DETECTED");
    SPDLOG_WARN("========================================");

    int critical_count = 0;
    int warning_count = 0;

    for (const auto& alert : alerts)
    {
      if (alert.severity == "critical")
        critical_count++;
      else if (alert.severity == "warning")
        warning_count++;

      std::string color = alert.GetSeverityColor();
      std::string reset = "\033[0m";

      SPDLOG_WARN("{}[{}] {}{}", color, alert.severity, alert.benchmark_name, reset);
      SPDLOG_WARN("  Baseline: {:.2f}", alert.baseline_value);
      SPDLOG_WARN("  Current:  {:.2f}", alert.current_value);
      SPDLOG_WARN("  Change:   {:+.2f}%", alert.change_pct);
    }

    SPDLOG_WARN("========================================");
    SPDLOG_WARN("Summary: {} critical, {} warning", critical_count, warning_count);
    SPDLOG_WARN("========================================");
  }

  /**
   * @brief Check if CI/CD should fail based on alerts
   */
  bool ShouldFailCI(const std::vector<RegressionAlert>& alerts) const
  {
    for (const auto& alert : alerts)
    {
      if (alert.severity == "critical" && config_.fail_on_critical)
        return true;
      if (alert.severity == "warning" && config_.fail_on_warning)
        return true;
    }
    return false;
  }

  /**
   * @brief Get baseline statistics
   */
  const std::map<std::string, BenchmarkBaseline>& GetBaselines() const
  {
    return baselines_;
  }

private:
  Config config_;
  std::map<std::string, BenchmarkBaseline> baselines_;
};

} // namespace carbio::performance_tests
