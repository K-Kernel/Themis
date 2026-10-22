#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

// Skeleton for include/themis/opt.hpp. Every body below throws "not written
// yet". Replace them in step order. Parameter names are commented out so the
// stubs compile without unused-parameter warnings; uncomment each name when you
// write the body.

namespace themis {

enum class Direction { Min, Max };

// One thing to optimise. evaluate(i) returns its value for candidate i.
// evaluate() below calls it from several threads at once, so it must only read
// shared data.
struct Objective {
  std::string name{};
  Direction direction{Direction::Min};
  std::function<double(size_t)> evaluate{};
};

// Every objective's value for every candidate. One row per candidate, one
// column per objective, stored row-major: with m = directions.size(), candidate
// i owns values[i * m] up to values[i * m + m - 1].
struct Scores {
  std::vector<Direction> directions{};
  std::vector<double> values{};

  size_t num_objectives() const { return directions.size(); }

  size_t num_candidates() const {
    if (!num_objectives()) {
      return 0;
    }

    return values.size() / num_objectives();
  }

  std::span<const double> row(size_t i) const {
    if (i >= num_candidates()) {
      throw std::out_of_range("index bigger than number of objectives");
    }

    return std::span<const double>(values).subspan(i * directions.size(),
                                                   directions.size());
  }
};

// Step 2. True when a is at least as good as b on every objective and strictly
// better on at least one. Good means smaller for Min and larger for Max.
// std::invalid_argument when a, b and directions differ in size.
inline bool dominates(std::span<const double> a, std::span<const double> b,
                      std::span<const Direction> directions) {
  if (a.size() != b.size()) {
    throw std::invalid_argument(" a and b differ in size");
  }
  if (directions.size() != a.size()) {
    throw std::invalid_argument("a and objectives differ in size");
  }

  bool at_least_good{true};
  bool strictly_better{false};
  for (size_t i{0}; i < a.size(); ++i) {
    if (directions[i] == Direction::Min && a[i] > b[i]) {
      return false;
    } else if (directions[i] == Direction::Max && a[i] < b[i]) {
      return false;
    }

    if (directions[i] == Direction::Min && a[i] < b[i]) {
      strictly_better = true;
    } else if (directions[i] == Direction::Max && a[i] > b[i]) {
      strictly_better = true;
    }
  }
  return at_least_good && strictly_better;
}

namespace detail {
// Step 3. std::invalid_argument, with a message starting with `who`, when there
// are no objectives, when values is not a whole number of rows, or when any
// value is NaN.
inline void check_scores(const Scores &s, const std::string &who) {
  if (s.directions.empty()) {
    throw std::invalid_argument(who + "objectives are empty");
  }

  if (s.values.size() % s.num_objectives() != 0) {
    throw std::invalid_argument(who + "values is not a whole number of rows");
  }

  if (std::any_of(s.values.begin(), s.values.end(),
                  [](double x) { return std::isnan(x); })) {
    throw std::invalid_argument(who + "NaN is present");
  }
}
} // namespace detail

// Step 3. Indices of the candidates no other candidate dominates, in ascending
// order. Calls detail::check_scores(s, "pareto_front") first.
inline std::vector<size_t> pareto_front(const Scores &s) {
  detail::check_scores(s, "pareto font");
  std::vector<size_t> front{};

  for (size_t i{0}; i < s.num_candidates(); ++i) {
    bool is_dominated = false;

    // if any member in front dominates i skip it
    for (size_t member : front) {
      if (dominates(s.row(member), s.row(i), s.directions)) {
        is_dominated = true;
        break;
      }
    }
    if (is_dominated) {
      continue;
    }
    std::erase_if(front, [&](size_t j) {
      return dominates(s.row(i), s.row(j), s.directions);
    });
    front.push_back(i);
  }
  return front;
}

// Step 4. Rescales every objective over the front members: 0 is the best value
// found on the front, 1 the worst, and an objective with the same value
// everywhere maps to 0. Returns front.size() rows of m values, row r belonging
// to candidate front[r]. Calls detail::check_scores(s, "normalise") first.
// std::invalid_argument for an empty front or a front value that is not finite;
// std::out_of_range for an index past the last candidate.
inline std::vector<double> normalise(const Scores &s,
                                     std::span<const size_t> front) {
  detail::check_scores(s, "normalise");

  if (front.empty()) {
    throw std::invalid_argument("front is empty");
  }

  std::span<const double> first_element{s.row(front[0])};
  std::vector<double> best(first_element.begin(), first_element.end());
  std::vector<double> worst(first_element.begin(), first_element.end());

  for (size_t member : front) {
    for (size_t j{0}; j < s.num_objectives(); ++j) {
      double value{s.row(member)[j]};
      if (std::isinf(value)) {
        throw std::invalid_argument("One of the front values is infinite");
      }

      if (s.directions[j] == Direction::Max) {
        if (value > best[j]) {
          best[j] = value;
        }
        if (value < worst[j]) {
          worst[j] = value;
        }
      } else {
        if (value < best[j]) {
          best[j] = value;
        }
        if (value > worst[j]) {
          worst[j] = value;
        }
      }
    }
  }

  std::vector<double> normalise{};
  normalise.reserve(front.size() * s.num_objectives());

  for (size_t i : front) {
    for (size_t j{0}; j < s.num_objectives(); ++j) {
      if (best[j] == worst[j]) {
        normalise.push_back(0.0);
      } else {
        normalise.push_back((s.row(i)[j] - best[j]) / (worst[j] - best[j]));
      }
    }
  }

  return normalise;
}

enum class Method { WeightedSum, Chebyshev };

// Step 5. The front member with the lowest score, where n is its normalised

// row:
//   WeightedSum: w[0] * n[0] + w[1] * n[1] + ...
//   Chebyshev:   the largest of w[k] * n[k]
// Ties go to the smaller n[0] + n[1] + ... (unweighted), then to the earlier
// front member. Returns the candidate index, not the position in front.
// std::invalid_argument when the number of weights differs from the number of
// objectives, when a weight is negative, NaN or infinite, or when every weight
// is 0.
inline size_t pick(const Scores &s, std::span<const size_t> front,
                   std::span<const double> weights, Method method) {

  bool all_zeros{true};
  if (weights.size() != s.num_objectives()) {
    throw std::invalid_argument("Incorrect number of weights");
  }

  for (double w : weights) {
    if (w < 0 || std::isnan(w) || std::isinf(w)) {
      throw std::invalid_argument("weight its invalid");
    } else if (w != 0) {
      all_zeros = false;
    }
  }
  if (all_zeros) {
    throw std::invalid_argument("all zero weight");
  }

  std::size_t m = s.num_objectives();
  double best_score{std::numeric_limits<double>::infinity()};
  double best_total{std::numeric_limits<double>::infinity()};
  std::size_t best_r{0};
  std::vector<double> normalised = normalise(s, front);

  for (size_t r{0}; r < front.size(); ++r) {
    double score{0};
    double total{0};

    for (size_t j{0}; j < m; ++j) {
      double n = normalised[r * m + j];
      double w = weights[j] * n;

      total += n;

      if (method == Method::WeightedSum) {
        score += w;
      } else {
        score = std::max(score, w);
      }
    }

    if (score < best_score || (score == best_score && total < best_total)) {
      best_score = score;
      best_total = total;
      best_r = r;
    }
  }

  return front[best_r];
}

// Step 6. Calls every objective on candidates 0 to num_candidates - 1 and
// returns the Scores. std::invalid_argument, before evaluating anything, when
// there are no objectives or when an objective has an empty evaluate. Runs
// candidates in parallel with OpenMP; when an objective throws, evaluate
// rethrows that exception after the loop.

inline Scores evaluate(std::span<const Objective> objectives,
                       size_t num_candidates) {
  if (objectives.empty()) {
    throw std::invalid_argument("evaluate: no objectives");
  }

  std::vector<Direction> directions{};
  for (const Objective &o : objectives) {
    if (!o.evaluate) {
      throw std::invalid_argument("evaluate: objective " + o.name +
                                  " has no function");
    }
    directions.push_back(o.direction);
  }

  const size_t m = objectives.size();
  std::vector<double> values(num_candidates * m);
  std::exception_ptr error = nullptr;

#pragma omp parallel for
  for (size_t i = 0; i < num_candidates; ++i) {
    try {
      for (size_t k = 0; k < m; ++k) {
        values[i * m + k] = objectives[k].evaluate(i);
      }
    } catch (...) {
#pragma omp critical
      {
        if (!error) {
          error = std::current_exception();
        }
      }
    }
  }

  if (error) {
    std::rethrow_exception(error);
  }
  return Scores{directions, values};
}

} // namespace themis
