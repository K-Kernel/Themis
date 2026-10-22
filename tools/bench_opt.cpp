// themis-bench-opt
// 10,000 random points in the unit square, scored on their distance to three fixed sites.
// Every point inside the triangle of sites is Pareto-optimal, so the front is large (2364
// points). Uniform random scores on three objectives give a front of about 60.
#include <themis/opt.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
static double ms_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

int main() {
  const size_t n = 10000;
  // mt19937_64 produces the same numbers on every platform; uniform_real_distribution does not,
  // so the doubles in [0, 1) are made by hand from the top 53 bits.
  std::mt19937_64 rng(42);
  auto uniform = [&rng] { return static_cast<double>(rng() >> 11) * 0x1.0p-53; };
  std::vector<double> x(n), y(n);
  for (size_t i = 0; i < n; ++i) {
    x[i] = uniform();
    y[i] = uniform();
  }

  const double site_x[3] = {0.2, 0.8, 0.5};
  const double site_y[3] = {0.2, 0.3, 0.9};
  std::vector<themis::Objective> objectives;
  for (size_t k = 0; k < 3; ++k) {
    objectives.push_back({"distance to site " + std::to_string(k), themis::Direction::Min,
                          [&x, &y, sx = site_x[k], sy = site_y[k]](size_t i) {
                            return std::hypot(x[i] - sx, y[i] - sy);
                          }});
  }

  double eval_ms = 1e300, front_ms = 1e300;
  themis::Scores scores;
  std::vector<size_t> front;
  for (int run = 0; run < 5; ++run) {
    auto t0 = Clock::now();
    scores = themis::evaluate(objectives, n);
    eval_ms = std::min(eval_ms, ms_since(t0));
    t0 = Clock::now();
    front = themis::pareto_front(scores);
    front_ms = std::min(front_ms, ms_since(t0));
  }

  const int picks = 1000;
  std::vector<std::vector<double>> weights(picks, std::vector<double>(3));
  for (auto &w : weights) {
    for (double &v : w) {
      v = uniform();
    }
  }
  size_t checksum = 0;
  double pick_ms[2];
  const themis::Method methods[2] = {themis::Method::WeightedSum, themis::Method::Chebyshev};
  for (int m = 0; m < 2; ++m) {
    auto t0 = Clock::now();
    for (const auto &w : weights) {
      checksum += themis::pick(scores, front, w, methods[m]);
    }
    pick_ms[m] = ms_since(t0) / picks;
  }

  std::printf("%zu candidates, %zu objectives\n", scores.num_candidates(), scores.num_objectives());
  std::printf("evaluate (best of 5)      %8.2f ms\n", eval_ms);
  std::printf("pareto front (best of 5)  %8.2f ms   (%zu on the front)\n", front_ms, front.size());
  std::printf("pick, weighted sum        %8.3f ms per call\n", pick_ms[0]);
  std::printf("pick, chebyshev           %8.3f ms per call\n", pick_ms[1]);
  std::printf("checksum %zu\n", checksum);
  return 0;
}
