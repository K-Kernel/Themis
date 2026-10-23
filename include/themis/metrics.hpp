#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

// Skeleton for include/themis/metrics.hpp. Every body below throws "not written
// yet". Replace them in step order, uncommenting each parameter name as you go,
// and delete this note when all of them are written.
//
// Each function summarises one list of numbers, such as every traveller's trip
// length for one candidate. All of them throw std::invalid_argument when xs is
// empty or holds a NaN or an infinity. An objective that can meet an
// unreachable candidate checks for that itself and returns UNREACHABLE instead
// of calling these.

namespace themis {

namespace detail {

inline void check_values(std::span<const double> xs, const std::string &who) {

  if (xs.empty()) {
    throw std::invalid_argument(who + "is empty");
  }
  for (const double &x : xs) {
    if (std::isnan(x) || std::isinf(x)) {
      throw std::invalid_argument(who + "values are NaN or infinite");
    }
  }
}

// Step 4. check_values, plus std::invalid_argument when any value is negative.
// Gini and Jain measure how evenly something is shared, which needs shares >=
// 0.
inline void check_shares(std::span<const double> xs, const std::string &who) {
  check_values(xs, who);
  for (const double &x : xs) {
    if (x < 0) {
      throw std::invalid_argument(who + "value is negative");
    }
  }
}
} // namespace detail

// Step 1. Sum divided by count.
inline double mean(std::span<const double> xs) {
  detail::check_values(xs, "mean");
  return std::accumulate(xs.begin(), xs.end(), 0.0) /
         static_cast<double>(xs.size());
}

// Step 2. Population variance: the mean of (x - mean)^2, dividing by n, the
// same as numpy.var. Compute the mean first, then a second loop for the
// squares.
inline double variance(std::span<const double> xs) {
  detail::check_values(xs, "variance");
  double mean_result{mean(xs)};
  double sum{0};

  for (const double &x : xs) {
    sum += std::pow(x - mean_result, 2);
  }

  return sum / static_cast<double>(xs.size());
}

// Step 2. Square root of the variance.
inline double stddev(std::span<const double> xs) {
  detail::check_values(xs, "standard deviation");
  return std::sqrt(variance(xs));
}

// Step 3. The p-th percentile, p from 0 to 100, as numpy.percentile computes it
// by default. Sort a copy; rank = p / 100 * (n - 1); with lo = floor(rank) and
// hi = ceil(rank), return sorted[lo] + (rank - lo) * (sorted[hi] - sorted[lo]).
// std::invalid_argument when p is outside [0, 100] or NaN.
inline double percentile(std::span<const double> xs, double p) {

  if (p < 0 || p > 100 || std::isnan(p)) {
    throw std::invalid_argument("p it is outside the range");
  }
  detail::check_values(xs, "percentile");
  std::vector<double> xs_copy{xs.begin(), xs.end()};
  std::sort(xs_copy.begin(), xs_copy.end());

  double rank{p / 100 * (static_cast<double>(xs.size()) - 1)};
  double lo = std::floor(rank);
  double hi = ceil(rank);

  return xs_copy[static_cast<size_t>(lo)] +
         (rank - lo) * (xs_copy[static_cast<size_t>(hi)] -
                        xs_copy[static_cast<size_t>(lo)]);
}

// Step 3. percentile(xs, 50).
inline double median(std::span<const double> xs) {
  detail::check_values(xs, "median");
  return percentile(xs, 50);
}

// Step 4. Gini coefficient of shares >= 0: the mean absolute difference between
// every pair of values divided by twice the mean. 0 when everyone has the same,
// (n - 1) / n when one value holds everything. Sorted ascending and numbered
// i = 1..n it equals  sum of (2i - n - 1) * x_i  divided by  n * sum of x_i,
// which is O(n log n) instead of looping over every pair. 0 when all are 0.
inline double gini(std::span<const double> xs) {
  detail::check_shares(xs, "Gini coefficient");

  bool all_zero{true};
  for (const double &x : xs) {
    if (x != 0) {
      all_zero = false;
      break;
    }
  }

  if (all_zero) {
    return 0.0;
  }

  std::vector<double> xs_copy{xs.begin(), xs.end()};
  std::sort(xs_copy.begin(), xs_copy.end());
  double numerator{0.0};

  for (size_t i{0}; i < xs.size(); ++i) {
    numerator += (static_cast<double>(2 * (i + 1)) -
                  static_cast<double>(xs.size()) - 1) *
                 xs_copy[i];
  }

  double denominator{static_cast<double>(xs.size()) *
                     (std::accumulate(xs.begin(), xs.end(), 0.0))};

  return numerator / denominator;
}

// Step 5. Jain's fairness index of shares >= 0: (sum of x)^2 / (n * sum of
// x^2). 1 when everyone has the same, 1 / n when one value holds everything. 1
// when all are 0.
inline double jain(std::span<const double> xs) {
  detail::check_shares(xs, "Jain's fairness");

  bool all_zero{true};
  for (const double &x : xs) {
    if (x != 0) {
      all_zero = false;
      break;
    }
  }

  if (all_zero) {
    return 1.0;
  }

  double x_square{};
  for (const double &x : xs) {
    x_square += x * x;
  }

  return (std::pow(std::accumulate(xs.begin(), xs.end(), 0.0), 2) /
          (static_cast<double>(xs.size()) * x_square));
}

} // namespace themis
