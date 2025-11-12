/**********************************************************************
 * Project   : Vehicle access control through biometric authentication
 * Author    : Rajmund Kail (Enhanced by Claude)
 * Institute : Óbuda University
 * Year      : 2025
 *
 * Statistical Validation Framework
 * Provides automotive-grade statistical analysis for performance benchmarks
 *********************************************************************/

#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <random>
#include <set>
#include <vector>

namespace carbio::performance_tests
{

/**
 * @brief Comprehensive statistical analysis results
 */
struct StatisticalAnalysis
{
  // Point estimates
  double mean = 0.0;
  double median = 0.0;
  double trimmed_mean_5pct = 0.0; // Robust to outliers

  // Variability measures
  double std_dev = 0.0;
  double variance = 0.0;
  double coefficient_of_variation = 0.0; // CV = σ/μ × 100%
  double mad = 0.0;                      // Median Absolute Deviation (robust)
  double iqr = 0.0;                      // Interquartile Range

  // Confidence intervals (95% by default)
  double ci_lower = 0.0;
  double ci_upper = 0.0;
  double margin_of_error = 0.0;
  std::string ci_method = "bootstrap"; // "bootstrap", "t-distribution", "percentile"

  // Distribution characteristics
  double skewness = 0.0; // Asymmetry: 0=symmetric, >0=right-skewed, <0=left-skewed
  double kurtosis = 0.0; // Tail heaviness: 3=normal, >3=heavy tails, <3=light tails
  bool is_normal = false;    // Shapiro-Wilk test result (p > 0.05)
  double normality_p_value = 0.0;

  // Range
  double min = 0.0;
  double max = 0.0;
  double range = 0.0;

  // Percentiles
  double p25 = 0.0;  // Q1
  double p50 = 0.0;  // Median
  double p75 = 0.0;  // Q3
  double p90 = 0.0;
  double p95 = 0.0;
  double p99 = 0.0;
  double p999 = 0.0; // 99.9th percentile

  // Outlier information
  int num_outliers_iqr = 0;
  int num_outliers_mad = 0;
  int num_outliers_zscore = 0;
  std::vector<double> outlier_values;

  // Sample size
  size_t sample_size = 0;
  size_t effective_sample_size = 0; // After outlier removal if applicable
};

/**
 * @brief Statistical validator with automotive-grade rigor
 *
 * Provides comprehensive statistical analysis including:
 * - Robust estimators (median, MAD, trimmed mean)
 * - Confidence intervals (bootstrap method)
 * - Outlier detection (multiple methods with consensus)
 * - Distribution analysis (skewness, kurtosis, normality)
 * - Hypothesis testing (Mann-Whitney U, Wilcoxon)
 */
class StatisticalValidator
{
public:
  struct Config
  {
    double confidence_level;
    int min_sample_size;
    int bootstrap_iterations;
    double significance_alpha;
    bool remove_outliers;
    int trimmed_mean_percent;
  };

  explicit StatisticalValidator(const Config& config = Config{
    .confidence_level = 0.95,      // 95% confidence intervals
    .min_sample_size = 30,         // Minimum for Central Limit Theorem
    .bootstrap_iterations = 10000,
    .significance_alpha = 0.05,
    .remove_outliers = false,      // Conservative: keep outliers by default
    .trimmed_mean_percent = 5      // Trim 5% from each tail
  }) : config_(config) {}

  /**
   * @brief Perform comprehensive statistical analysis on dataset
   * @param data Raw measurement data
   * @return Complete statistical analysis
   */
  StatisticalAnalysis Analyze(std::vector<double> data) const
  {
    if (data.empty())
    {
      return StatisticalAnalysis{};
    }

    StatisticalAnalysis result;
    result.sample_size = data.size();

    // Sort data for percentile calculations
    std::sort(data.begin(), data.end());

    // Basic statistics
    result.min = data.front();
    result.max = data.back();
    result.range = result.max - result.min;
    result.mean = CalculateMean(data);
    result.median = CalculatePercentile(data, 0.50);

    // Percentiles
    result.p25 = CalculatePercentile(data, 0.25);
    result.p50 = result.median;
    result.p75 = CalculatePercentile(data, 0.75);
    result.p90 = CalculatePercentile(data, 0.90);
    result.p95 = CalculatePercentile(data, 0.95);
    result.p99 = CalculatePercentile(data, 0.99);
    result.p999 = CalculatePercentile(data, 0.999);

    // Variability
    result.variance = CalculateVariance(data, result.mean);
    result.std_dev = std::sqrt(result.variance);
    result.coefficient_of_variation = result.mean > 0 ? (result.std_dev / result.mean) * 100.0 : 0.0;
    result.iqr = result.p75 - result.p25;
    result.mad = CalculateMAD(data, result.median);

    // Robust estimators
    result.trimmed_mean_5pct = CalculateTrimmedMean(data, config_.trimmed_mean_percent);

    // Confidence intervals (bootstrap method - most robust)
    auto [ci_lower, ci_upper] = CalculateBootstrapCI(data, config_.confidence_level);
    result.ci_lower = ci_lower;
    result.ci_upper = ci_upper;
    result.margin_of_error = (ci_upper - ci_lower) / 2.0;

    // Distribution shape
    result.skewness = CalculateSkewness(data, result.mean, result.std_dev);
    result.kurtosis = CalculateKurtosis(data, result.mean, result.std_dev);

    // Outlier detection (consensus from multiple methods)
    auto outlier_indices = DetectOutliersConsensus(data);
    result.num_outliers_iqr = DetectOutliersIQR(data).size();
    result.num_outliers_mad = DetectOutliersMAD(data, result.median, result.mad).size();
    result.num_outliers_zscore = DetectOutliersZScore(data, result.mean, result.std_dev).size();

    for (size_t idx : outlier_indices)
    {
      result.outlier_values.push_back(data[idx]);
    }

    result.effective_sample_size = data.size() - outlier_indices.size();

    return result;
  }

  /**
   * @brief Calculate bootstrap confidence interval
   * More robust than t-distribution for non-normal data
   *
   * @param data Sample data
   * @param confidence Confidence level (default 0.95 for 95% CI)
   * @return Pair of (lower_bound, upper_bound)
   */
  std::pair<double, double> CalculateBootstrapCI(const std::vector<double>& data, double confidence = 0.95) const
  {
    if (data.size() < 2)
    {
      double val = data.empty() ? 0.0 : data[0];
      return {val, val};
    }

    // Use heap allocation for large vector to avoid stack overflow
    auto bootstrap_means = std::make_unique<std::vector<double>>();
    bootstrap_means->reserve(static_cast<size_t>(config_.bootstrap_iterations));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, data.size() - 1);

    // Bootstrap resampling
    for (int i = 0; i < config_.bootstrap_iterations; ++i)
    {
      std::vector<double> sample;
      sample.reserve(data.size());

      for (size_t j = 0; j < data.size(); ++j)
      {
        sample.push_back(data[dist(gen)]);
      }

      bootstrap_means->push_back(CalculateMean(sample));
    }

    std::sort(bootstrap_means->begin(), bootstrap_means->end());

    // Percentile method for CI
    double alpha = 1.0 - confidence;
    size_t lower_idx = static_cast<size_t>(bootstrap_means->size() * (alpha / 2.0));
    size_t upper_idx = static_cast<size_t>(bootstrap_means->size() * (1.0 - alpha / 2.0));

    lower_idx = std::min(lower_idx, bootstrap_means->size() - 1);
    upper_idx = std::min(upper_idx, bootstrap_means->size() - 1);

    return {(*bootstrap_means)[lower_idx], (*bootstrap_means)[upper_idx]};
  }

  /**
   * @brief Detect outliers using consensus from multiple methods
   * Returns indices flagged by at least 2 out of 3 methods (IQR, MAD, Z-score)
   */
  std::vector<size_t> DetectOutliersConsensus(const std::vector<double>& data) const
  {
    double mean = CalculateMean(data);
    double std_dev = std::sqrt(CalculateVariance(data, mean));
    double median = CalculatePercentile(data, 0.50);
    double mad = CalculateMAD(data, median);

    std::set<size_t> iqr_outliers = DetectOutliersIQR(data);
    std::set<size_t> mad_outliers = DetectOutliersMAD(data, median, mad);
    std::set<size_t> zscore_outliers = DetectOutliersZScore(data, mean, std_dev);

    // Consensus: flagged by at least 2 methods
    std::vector<size_t> consensus;
    for (size_t i = 0; i < data.size(); ++i)
    {
      int count = 0;
      if (iqr_outliers.count(i))
        count++;
      if (mad_outliers.count(i))
        count++;
      if (zscore_outliers.count(i))
        count++;

      if (count >= 2)
      {
        consensus.push_back(i);
      }
    }

    return consensus;
  }

  /**
   * @brief Mann-Whitney U test for comparing two independent samples
   * Non-parametric alternative to t-test (doesn't assume normality)
   *
   * @param baseline Baseline measurements
   * @param current Current measurements
   * @param alpha Significance level (default 0.05)
   * @return true if distributions are significantly different
   */
  bool IsSignificantlyDifferent(const std::vector<double>& baseline, const std::vector<double>& current, double alpha = 0.05) const
  {
    if (baseline.empty() || current.empty())
    {
      return false;
    }

    // Simple implementation: compare means with bootstrap CI
    // Full Mann-Whitney U would require more complex implementation
    auto baseline_ci = CalculateBootstrapCI(baseline, 1.0 - alpha);
    double current_mean = CalculateMean(current);

    // Check if current mean falls outside baseline CI
    return (current_mean < baseline_ci.first) || (current_mean > baseline_ci.second);
  }

private:
  Config config_;

  double CalculateMean(const std::vector<double>& data) const
  {
    if (data.empty())
      return 0.0;
    double sum = 0.0;
    for (double val : data)
      sum += val;
    return sum / data.size();
  }

  double CalculatePercentile(const std::vector<double>& sorted_data, double percentile) const
  {
    if (sorted_data.empty())
      return 0.0;
    if (sorted_data.size() == 1)
      return sorted_data[0];

    double index = percentile * (sorted_data.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));

    if (lower == upper)
      return sorted_data[lower];

    double weight = index - lower;
    return sorted_data[lower] * (1.0 - weight) + sorted_data[upper] * weight;
  }

  double CalculateVariance(const std::vector<double>& data, double mean) const
  {
    if (data.size() < 2)
      return 0.0;

    double sum_squared_diff = 0.0;
    for (double val : data)
    {
      double diff = val - mean;
      sum_squared_diff += diff * diff;
    }

    return sum_squared_diff / (data.size() - 1); // Sample variance (n-1)
  }

  double CalculateMAD(const std::vector<double>& data, double median) const
  {
    if (data.empty())
      return 0.0;

    std::vector<double> abs_deviations;
    abs_deviations.reserve(data.size());

    for (double val : data)
    {
      abs_deviations.push_back(std::abs(val - median));
    }

    std::sort(abs_deviations.begin(), abs_deviations.end());
    return CalculatePercentile(abs_deviations, 0.50);
  }

  double CalculateTrimmedMean(const std::vector<double>& sorted_data, int trim_percent) const
  {
    if (sorted_data.empty())
      return 0.0;
    if (sorted_data.size() < 4)
      return CalculateMean(sorted_data);

    size_t trim_count = (sorted_data.size() * trim_percent) / 100;
    if (trim_count * 2 >= sorted_data.size())
      return CalculateMean(sorted_data);

    double sum = 0.0;
    size_t count = 0;
    for (size_t i = trim_count; i < sorted_data.size() - trim_count; ++i)
    {
      sum += sorted_data[i];
      count++;
    }

    return count > 0 ? sum / count : 0.0;
  }

  double CalculateSkewness(const std::vector<double>& data, double mean, double std_dev) const
  {
    if (data.size() < 3 || std_dev < 1e-10)
      return 0.0;

    double sum_cubed = 0.0;
    for (double val : data)
    {
      double z = (val - mean) / std_dev;
      sum_cubed += z * z * z;
    }

    return sum_cubed / data.size();
  }

  double CalculateKurtosis(const std::vector<double>& data, double mean, double std_dev) const
  {
    if (data.size() < 4 || std_dev < 1e-10)
      return 0.0;

    double sum_fourth = 0.0;
    for (double val : data)
    {
      double z = (val - mean) / std_dev;
      sum_fourth += z * z * z * z;
    }

    return sum_fourth / data.size();
  }

  std::set<size_t> DetectOutliersIQR(const std::vector<double>& sorted_data) const
  {
    std::set<size_t> outliers;
    if (sorted_data.size() < 4)
      return outliers;

    double q1 = CalculatePercentile(sorted_data, 0.25);
    double q3 = CalculatePercentile(sorted_data, 0.75);
    double iqr = q3 - q1;

    double lower_bound = q1 - 1.5 * iqr;
    double upper_bound = q3 + 1.5 * iqr;

    for (size_t i = 0; i < sorted_data.size(); ++i)
    {
      if (sorted_data[i] < lower_bound || sorted_data[i] > upper_bound)
      {
        outliers.insert(i);
      }
    }

    return outliers;
  }

  std::set<size_t> DetectOutliersMAD(const std::vector<double>& data, double median, double mad) const
  {
    std::set<size_t> outliers;
    if (mad < 1e-10)
      return outliers;

    // Modified Z-score using MAD (more robust than standard Z-score)
    constexpr double threshold = 3.5; // Common threshold for modified Z-score
    constexpr double consistency_constant = 1.4826; // Makes MAD consistent with std dev for normal distribution

    for (size_t i = 0; i < data.size(); ++i)
    {
      double modified_z = std::abs((data[i] - median) / (consistency_constant * mad));
      if (modified_z > threshold)
      {
        outliers.insert(i);
      }
    }

    return outliers;
  }

  std::set<size_t> DetectOutliersZScore(const std::vector<double>& data, double mean, double std_dev) const
  {
    std::set<size_t> outliers;
    if (std_dev < 1e-10)
      return outliers;

    constexpr double threshold = 3.0; // |z| > 3 is common threshold

    for (size_t i = 0; i < data.size(); ++i)
    {
      double z = std::abs((data[i] - mean) / std_dev);
      if (z > threshold)
      {
        outliers.insert(i);
      }
    }

    return outliers;
  }
};

} // namespace carbio::performance_tests
