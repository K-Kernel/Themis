#include <doctest/doctest.h>
#include <limits>
#include <span>
#include <stdexcept>
#include <themis/metrics.hpp>
#include <themis/opt.hpp>
#include <vector>

using namespace themis;

namespace {
using V = std::vector<double>;
const double INF = std::numeric_limits<double>::infinity();
const double NaN = std::numeric_limits<double>::quiet_NaN();
} // namespace

TEST_CASE("mean") {
  CHECK(mean(V{1, 2, 3, 4}) == 2.5);
  CHECK(mean(V{5}) == 5);
  CHECK(mean(V{-1, 1}) == 0);
}

TEST_CASE("variance and stddev divide by n") {
  // The textbook set: mean 5, squared deviations 9 1 1 1 0 0 4 16, sum 32, 32 / 8 = 4.
  CHECK(variance(V{2, 4, 4, 4, 5, 5, 7, 9}) == doctest::Approx(4));
  CHECK(stddev(V{2, 4, 4, 4, 5, 5, 7, 9}) == doctest::Approx(2));
  CHECK(variance(V{5}) == 0);
  CHECK(stddev(V{5}) == 0);
}

TEST_CASE("percentile interpolates between sorted values") {
  // Sorted: 1 2 3 4. The rank is p / 100 * (n - 1), so p = 25 gives rank 0.75:
  // three quarters of the way from 1 to 2, which is 1.75.
  const V xs{4, 1, 3, 2};
  CHECK(percentile(xs, 0) == 1);
  CHECK(percentile(xs, 25) == doctest::Approx(1.75));
  CHECK(percentile(xs, 50) == doctest::Approx(2.5));
  CHECK(percentile(xs, 100) == 4);
  CHECK(percentile(V{10, 20, 30, 40, 50}, 90) == doctest::Approx(46));
  CHECK(percentile(V{7}, 37) == 7);
}

TEST_CASE("median") {
  CHECK(median(V{3, 1, 2}) == 2);
  CHECK(median(V{4, 1, 3, 2}) == doctest::Approx(2.5));
}

TEST_CASE("gini: 0 for equal shares, (n - 1) / n when one value holds everything") {
  CHECK(gini(V{1, 1, 1, 1}) == doctest::Approx(0));
  CHECK(gini(V{0, 0, 0, 1}) == doctest::Approx(0.75));
  CHECK(gini(V{1, 2, 3, 4}) == doctest::Approx(0.25));
  CHECK(gini(V{4, 1, 3, 2}) == doctest::Approx(0.25));     // order does not matter
  CHECK(gini(V{10, 20, 30, 40}) == doctest::Approx(0.25)); // nor does scale
  CHECK(gini(V{0, 0, 0}) == 0);
  CHECK(gini(V{5}) == 0);
}

TEST_CASE("jain: 1 for equal shares, 1 / n when one value holds everything") {
  CHECK(jain(V{1, 1, 1, 1}) == doctest::Approx(1));
  CHECK(jain(V{1, 0, 0, 0}) == doctest::Approx(0.25));
  CHECK(jain(V{1, 2, 3, 4}) == doctest::Approx(100.0 / 120.0)); // 10 * 10 / (4 * 30)
  CHECK(jain(V{0, 0, 0}) == 1);
  CHECK(jain(V{5}) == 1);
}

TEST_CASE("metrics reject empty and non-finite input") {
  for (auto f : {&mean, &variance, &stddev, &median, &gini, &jain}) {
    CHECK_THROWS_AS(f(V{}), std::invalid_argument);
    CHECK_THROWS_AS(f(V{1, NaN}), std::invalid_argument);
    CHECK_THROWS_AS(f(V{1, INF}), std::invalid_argument);
  }
  CHECK_THROWS_AS(percentile(V{1, 2}, -1), std::invalid_argument);
  CHECK_THROWS_AS(percentile(V{1, 2}, 101), std::invalid_argument);
  CHECK_THROWS_AS(percentile(V{1, 2}, NaN), std::invalid_argument);
  CHECK_THROWS_AS(gini(V{1, -1}), std::invalid_argument);
  CHECK_THROWS_AS(jain(V{1, -1}), std::invalid_argument);
}

TEST_CASE("metrics as objectives: shortest average trip against the fairest split") {
  // Three candidate meeting points, each with the trip lengths of three people.
  //   candidate 0: 100 100 100   mean 100    gini 0
  //   candidate 1:  10  10 200   mean 73.3   gini 380 / 660
  //   candidate 2:  50  60 250   mean 120    gini 400 / 1080   (beaten by 0 on both)
  const std::vector<V> trips{{100, 100, 100}, {10, 10, 200}, {50, 60, 250}};
  std::vector<Objective> objectives{
      {"mean trip", Direction::Min, [&](size_t c) { return mean(trips[c]); }},
      {"gini", Direction::Min, [&](size_t c) { return gini(trips[c]); }},
  };
  Scores s = evaluate(objectives, trips.size());
  CHECK(s.row(1)[1] == doctest::Approx(380.0 / 660.0));
  std::vector<size_t> front = pareto_front(s);
  CHECK(front == std::vector<size_t>{0, 1});
  CHECK(pick(s, front, V{1, 0}, Method::Chebyshev) == 1);
  CHECK(pick(s, front, V{0, 1}, Method::Chebyshev) == 0);
}
