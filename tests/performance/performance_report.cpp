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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace carbio::performance_tests
{

/**
 * @brief Performance report generator for fingerprint authentication benchmarks
 *
 * Generates comprehensive performance reports including:
 * - Executive summary with key metrics
 * - Detailed latency analysis (percentiles, distribution)
 * - Real-time performance assessment (jitter, WCET, deadline adherence)
 * - Biometric accuracy metrics (FAR/FRR approximations, FTE/FTA rates)
 * - Throughput and scalability analysis
 * - Comparison against industry standards and automotive requirements
 */
class PerformanceReporter
{
public:
  struct LatencyMetrics
  {
    double min_ms = 0.0;
    double max_ms = 0.0;
    double mean_ms = 0.0;
    double median_ms = 0.0;
    double p90_ms = 0.0;
    double p95_ms = 0.0;
    double p99_ms = 0.0;
    double p999_ms = 0.0;
    double std_dev_ms = 0.0;
    double jitter_ms = 0.0;
    double wcet_ms = 0.0;
  };

  struct AccuracyMetrics
  {
    double success_rate_pct = 0.0;
    double frr_pct = 0.0; // False Rejection Rate (genuine samples rejected)
    double far_pct = 0.0; // False Acceptance Rate (impostor samples accepted)
    double fte_pct = 0.0; // Failure To Enroll
    double fta_pct = 0.0; // Failure To Acquire
    double mean_confidence = 0.0;
    int total_attempts = 0;
    int successful = 0;
    int failed = 0;
  };

  struct ThroughputMetrics
  {
    double ops_per_sec = 0.0;
    double ops_per_hour = 0.0;
    double avg_time_per_op_ms = 0.0;
  };

  static std::string GenerateFullReport(const std::string& benchmark_name, const LatencyMetrics& latency, const AccuracyMetrics& accuracy, const ThroughputMetrics& throughput, const std::vector<std::string>& additional_notes = {})
  {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);

    // Header
    report << "\n========================================\n";
    report << "PERFORMANCE REPORT: " << benchmark_name << "\n";
    report << "========================================\n\n";

    // Executive Summary
    report << "EXECUTIVE SUMMARY\n";
    report << "-----------------\n";
    report << "Total Operations: " << accuracy.total_attempts << "\n";
    report << "Success Rate:     " << accuracy.success_rate_pct << "%\n";
    report << "Median Latency:   " << latency.median_ms << " ms\n";
    report << "P95 Latency:      " << latency.p95_ms << " ms\n";
    report << "Throughput:       " << throughput.ops_per_sec << " ops/sec\n";
    report << "\n";

    // Real-Time Performance Analysis
    report << "REAL-TIME PERFORMANCE ANALYSIS\n";
    report << "------------------------------\n";
    report << "Latency Distribution:\n";
    report << "  Min:              " << latency.min_ms << " ms\n";
    report << "  Mean:             " << latency.mean_ms << " ms\n";
    report << "  Median (P50):     " << latency.median_ms << " ms\n";
    report << "  P90:              " << latency.p90_ms << " ms\n";
    report << "  P95:              " << latency.p95_ms << " ms\n";
    report << "  P99:              " << latency.p99_ms << " ms\n";
    report << "  P99.9:            " << latency.p999_ms << " ms\n";
    report << "  Max:              " << latency.max_ms << " ms\n";
    report << "\n";
    report << "Variability Metrics:\n";
    report << "  Std Deviation:    " << latency.std_dev_ms << " ms\n";
    report << "  Jitter:           " << latency.jitter_ms << " ms\n";
    report << "  WCET:             " << latency.wcet_ms << " ms\n";
    report << "  Coefficient of Variation: " << (latency.mean_ms > 0 ? (latency.std_dev_ms / latency.mean_ms) * 100.0 : 0.0) << "%\n";
    report << "\n";

    // Real-time Assessment
    report << "Real-Time System Assessment:\n";
    const double automotive_deadline_ms = 500.0; // Typical automotive HMI deadline
    double deadline_adherence = latency.wcet_ms <= automotive_deadline_ms ? 100.0 : (automotive_deadline_ms / latency.wcet_ms) * 100.0;
    report << "  Automotive Deadline (500ms): " << (latency.wcet_ms <= automotive_deadline_ms ? "PASS" : "FAIL") << "\n";
    report << "  Deadline Adherence:   " << deadline_adherence << "%\n";
    report << "  Jitter/Mean Ratio:    " << (latency.mean_ms > 0 ? (latency.jitter_ms / latency.mean_ms) * 100.0 : 0.0) << "%\n";
    report << "\n";

    // Biometric Accuracy Metrics
    report << "BIOMETRIC ACCURACY METRICS\n";
    report << "--------------------------\n";
    report << "Success Metrics:\n";
    report << "  Success Rate:     " << accuracy.success_rate_pct << "%\n";
    report << "  Successful Ops:   " << accuracy.successful << "\n";
    report << "  Failed Ops:       " << accuracy.failed << "\n";
    report << "\n";
    report << "Error Rates:\n";
    if (accuracy.frr_pct >= 0.0)
    {
      report << "  FRR (False Reject):   " << accuracy.frr_pct << "%\n";
    }
    if (accuracy.far_pct >= 0.0)
    {
      report << "  FAR (False Accept):   " << accuracy.far_pct << "%\n";
    }
    if (accuracy.fte_pct > 0.0)
    {
      report << "  FTE (Fail To Enroll): " << accuracy.fte_pct << "%\n";
    }
    if (accuracy.fta_pct > 0.0)
    {
      report << "  FTA (Fail To Acquire):" << accuracy.fta_pct << "%\n";
    }
    if (accuracy.mean_confidence > 0.0)
    {
      report << "  Mean Confidence:      " << accuracy.mean_confidence << "\n";
    }
    report << "\n";

    // Industry Standards Comparison
    report << "INDUSTRY STANDARDS COMPARISON\n";
    report << "-----------------------------\n";
    report << "ISO/IEC 19795 Benchmark:\n";
    report << "  FRR Target:       < 1%     [" << (accuracy.frr_pct < 1.0 ? "PASS" : "FAIL") << "]\n";
    report << "  FAR Target:       < 0.001% [" << (accuracy.far_pct < 0.001 ? "PASS" : "ESTIMATE") << "]\n";
    report << "  FTE Target:       < 3%     [" << (accuracy.fte_pct < 3.0 ? "PASS" : "FAIL") << "]\n";
    report << "\n";

    // Throughput Analysis
    report << "THROUGHPUT ANALYSIS\n";
    report << "-------------------\n";
    report << "  Operations/second:   " << throughput.ops_per_sec << "\n";
    report << "  Operations/hour:     " << throughput.ops_per_hour << "\n";
    report << "  Avg Time/Operation:  " << throughput.avg_time_per_op_ms << " ms\n";
    report << "\n";

    // Additional Notes
    if (!additional_notes.empty())
    {
      report << "ADDITIONAL NOTES\n";
      report << "----------------\n";
      for (const auto& note : additional_notes)
      {
        report << "  - " << note << "\n";
      }
      report << "\n";
    }

    report << "========================================\n\n";

    return report.str();
  }

  static void SaveReportToFile(const std::string& report, const std::string& filename)
  {
    std::ofstream file(filename);
    if (file.is_open())
    {
      file << report;
      file.close();
      SPDLOG_INFO("Performance report saved to: {}", filename);
    }
    else
    {
      SPDLOG_ERROR("Failed to save performance report to: {}", filename);
    }
  }
};

} // namespace carbio::performance_tests
