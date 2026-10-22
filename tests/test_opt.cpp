#include <algorithm>
#include <cmath>
#include <doctest/doctest.h>
#include <limits>
#include <stdexcept>
#include <themis/opt.hpp>
#include <vector>

using namespace themis;

namespace {
using V = std::vector<double>;
using Ids = std::vector<size_t>;
constexpr Direction Min = Direction::Min;
constexpr Direction Max = Direction::Max;
const double INF = std::numeric_limits<double>::infinity();
const double NaN = std::numeric_limits<double>::quiet_NaN();
const std::vector<Direction> MIN_MIN{Min, Min};

// The hand-checked set. Two objectives, both minimised.
//   candidate:  0      1      2      3      4      5      6
//   scores:   (1,5)  (2,3)  (3,4)  (4,1)  (2,3)  (5,5)  (1,6)
// 1 beats 2, 1 beats 5, 0 beats 6. 4 is a copy of 1, and equal rows never
// dominate each other.
const Scores HAND{{Min, Min}, {1, 5, 2, 3, 3, 4, 4, 1, 2, 3, 5, 5, 1, 6}};

void check_close(const V &got, const V &want) {
  REQUIRE(got.size() == want.size());
  for (size_t i = 0; i < got.size(); ++i) {
    CHECK(got[i] == doctest::Approx(want[i]));
  }
}
} // namespace

TEST_CASE("dominates: no worse everywhere and better somewhere") {
  CHECK(dominates(V{1, 2}, V{2, 3}, MIN_MIN));
  CHECK_FALSE(dominates(V{2, 3}, V{1, 2}, MIN_MIN));
  CHECK(dominates(V{1, 2}, V{1, 3}, MIN_MIN)); // a tie on one objective is fine
  CHECK_FALSE(
      dominates(V{1, 3}, V{2, 2}, MIN_MIN)); // a trade-off: neither wins
  CHECK_FALSE(dominates(V{2, 2}, V{1, 3}, MIN_MIN));
  CHECK_FALSE(dominates(V{1, 2}, V{1, 2}, MIN_MIN)); // equal rows
}

TEST_CASE("dominates: a Max objective prefers the larger value") {
  const std::vector<Direction> min_max{Min, Max};
  CHECK(dominates(V{1, 5}, V{2, 3}, min_max));
  CHECK_FALSE(dominates(V{2, 3}, V{1, 5}, min_max));
  CHECK_FALSE(dominates(V{1, 3}, V{2, 5}, min_max));
}

TEST_CASE("dominates: infinity is the worst possible score") {
  CHECK(dominates(V{1, 2}, V{1, INF}, MIN_MIN));
  CHECK_FALSE(dominates(V{INF, INF}, V{INF, INF}, MIN_MIN));
}

TEST_CASE("dominates rejects mismatched sizes") {
  CHECK_THROWS_AS(dominates(V{1, 2}, V{1}, MIN_MIN), std::invalid_argument);
  CHECK_THROWS_AS(dominates(V{1, 2}, V{1, 2}, std::vector<Direction>{Min}),
                  std::invalid_argument);
}

TEST_CASE("Scores: rows and counts") {
  CHECK(HAND.num_objectives() == 2);
  CHECK(HAND.num_candidates() == 7);
  CHECK(HAND.row(3)[0] == 4);
  CHECK(HAND.row(3)[1] == 1);
  CHECK_THROWS_AS(HAND.row(7), std::out_of_range);
}

TEST_CASE("pareto_front of the hand-checked set") {
  CHECK(pareto_front(HAND) == Ids{0, 1, 3, 4});
}

TEST_CASE("pareto_front follows the direction of each objective") {
  // The same numbers with the second objective maximised: candidate 6 (1, 6)
  // now beats everyone.
  Scores s{{Min, Max}, HAND.values};
  CHECK(pareto_front(s) == Ids{6});
}

TEST_CASE("pareto_front: one objective keeps every candidate tied for best") {
  CHECK(pareto_front(Scores{{Min}, {3, 1, 2, 1}}) == Ids{1, 3});
  CHECK(pareto_front(Scores{{Max}, {3, 1, 2, 1}}) == Ids{0});
}

TEST_CASE("pareto_front: unreachable candidates (infinite scores) drop out") {
  CHECK(pareto_front(Scores{{Min, Min}, {1, 2, INF, INF, 3, 0}}) == Ids{0, 2});
}

TEST_CASE("pareto_front edge cases") {
  CHECK(pareto_front(Scores{{Min, Min}, {}}).empty());
  CHECK(pareto_front(Scores{{Min}, {7}}) == Ids{0});
  CHECK(pareto_front(Scores{{Min, Max}, {1, 1, 1, 1, 1, 1}}) == Ids{0, 1, 2});
}

TEST_CASE("pareto_front rejects malformed scores") {
  CHECK_THROWS_AS(pareto_front(Scores{{}, {1, 2}}),
                  std::invalid_argument); // no objectives
  CHECK_THROWS_AS(pareto_front(Scores{{Min, Min}, {1, 2, 3}}),
                  std::invalid_argument); // 1.5 rows
  CHECK_THROWS_AS(pareto_front(Scores{{Min, Min}, {1, NaN, 3, 4}}),
                  std::invalid_argument);
}

TEST_CASE("normalise: 0 is the best value on the front and 1 the worst") {
  // Front {0, 1, 3, 4}. First objective: 1, 2, 4, 2 (best 1, worst 4).
  // Second objective: 5, 3, 1, 3 (best 1, worst 5).
  check_close(normalise(HAND, Ids{0, 1, 3, 4}),
              V{0, 1, 1.0 / 3, 0.5, 1, 0, 1.0 / 3, 0.5});
}

TEST_CASE("normalise: Max objectives and constant objectives") {
  // Second objective maximised: 20 is the best value, 10 the worst.
  check_close(normalise(Scores{{Min, Max}, {1, 10, 3, 20}}, Ids{0, 1}),
              V{0, 1, 1, 0});
  // Second objective is the same for everyone: it maps to 0.
  check_close(normalise(Scores{{Min, Min}, {1, 5, 2, 5}}, Ids{0, 1}),
              V{0, 0, 1, 0});
}

TEST_CASE("normalise rejects a bad front") {
  CHECK_THROWS_AS(normalise(HAND, Ids{}), std::invalid_argument);
  CHECK_THROWS_AS(normalise(HAND, Ids{0, 9}), std::out_of_range);
  CHECK_THROWS_AS(normalise(Scores{{Min, Min}, {1, INF, 2, 0}}, Ids{0, 1}),
                  std::invalid_argument);
}

TEST_CASE("normalise and pick skip infinite scores off the front") {
  // Candidate 1 is unreachable, so it is dominated and off the front. Only
  // the front members 0 and 2 are rescaled, so the infinity must not matter.
  Scores s{{Min, Min}, {1, 2, INF, INF, 3, 0}};
  Ids front = pareto_front(s);
  REQUIRE(front == Ids{0, 2});
  check_close(normalise(s, front), V{0, 1, 1, 0});
  CHECK(pick(s, front, V{1, 0}, Method::Chebyshev) == 0);
  CHECK(pick(s, front, V{0, 1}, Method::Chebyshev) == 2);
}

TEST_CASE(
    "pick: one positive weight picks the best candidate on that objective") {
  Ids front = pareto_front(HAND);
  for (Method m : {Method::WeightedSum, Method::Chebyshev}) {
    CHECK(pick(HAND, front, V{1, 0}, m) == 0);
    CHECK(pick(HAND, front, V{0, 1}, m) == 3);
  }
  Scores s{{Min, Max}, {1, 10, 3, 20}};
  CHECK(pick(s, Ids{0, 1}, V{1, 0}, Method::Chebyshev) == 0);
  CHECK(pick(s, Ids{0, 1}, V{0, 1}, Method::Chebyshev) == 1);
}

TEST_CASE(
    "pick: the weighted sum never reaches a point in a dent of the front") {
  // A = (0, 1), B = (0.6, 0.6), C = (1, 0). All three are on the front, but B
  // sits above the straight line from A to C: 0.6 * w1 + 0.6 * w2 > min(w1, w2)
  // for every weighting.
  Scores s{{Min, Min}, {0, 1, 0.6, 0.6, 1, 0}};
  Ids front = pareto_front(s);
  REQUIRE(front == Ids{0, 1, 2});
  for (int t = 0; t <= 100; ++t) {
    V w{static_cast<double>(t), static_cast<double>(100 - t)};
    CHECK(pick(s, front, w, Method::WeightedSum) != 1);
  }
  CHECK(pick(s, front, V{1, 0}, Method::Chebyshev) == 0);
  CHECK(pick(s, front, V{1, 1}, Method::Chebyshev) == 1);
  CHECK(pick(s, front, V{0, 1}, Method::Chebyshev) == 2);
}

TEST_CASE("pick: ties go to the smaller total, then to the earlier candidate") {
  // Chebyshev with weights (1, 1): candidates 0 and 1 both score 0.5.
  // Candidate 1 has the smaller total of normalised scores (0.6 against 0.7),
  // so it wins.
  Scores s{{Min, Min}, {0.5, 0.2, 0.1, 0.5, 0, 1, 1, 0}};
  Ids front = pareto_front(s);
  REQUIRE(front == Ids{0, 1, 2, 3});
  CHECK(pick(s, front, V{1, 1}, Method::Chebyshev) == 1);

  Scores twins{{Min, Min}, {1, 2, 1, 2}};
  CHECK(pick(twins, pareto_front(twins), V{1, 1}, Method::WeightedSum) == 0);
}

TEST_CASE("pick rejects bad weights and an empty front") {
  Scores s{{Min, Min}, {0, 1, 1, 0}};
  Ids front = pareto_front(s);
  CHECK_THROWS_AS(pick(s, front, V{1}, Method::WeightedSum),
                  std::invalid_argument);
  CHECK_THROWS_AS(pick(s, front, V{1, -1}, Method::WeightedSum),
                  std::invalid_argument);
  CHECK_THROWS_AS(pick(s, front, V{0, 0}, Method::WeightedSum),
                  std::invalid_argument);
  CHECK_THROWS_AS(pick(s, front, V{1, NaN}, Method::Chebyshev),
                  std::invalid_argument);
  CHECK_THROWS_AS(pick(s, front, V{1, INF}, Method::Chebyshev),
                  std::invalid_argument);
  CHECK_THROWS_AS(pick(s, Ids{}, V{1, 1}, Method::Chebyshev),
                  std::invalid_argument);
}

TEST_CASE("evaluate fills one row per candidate") {
  std::vector<Objective> objectives{
      {"twice", Min, [](size_t i) { return 2.0 * static_cast<double>(i); }},
      {"square", Max, [](size_t i) { return static_cast<double>(i * i); }},
  };
  Scores s = evaluate(objectives, 4);
  CHECK(s.directions == std::vector<Direction>{Min, Max});
  CHECK(s.values == V{0, 0, 2, 1, 4, 4, 6, 9});
  CHECK(s.num_candidates() == 4);
  CHECK(evaluate(objectives, 0).num_candidates() == 0);
}

TEST_CASE("evaluate covers every candidate exactly once") {
  const size_t n = 100000;
  std::vector<Objective> objectives{
      {"index", Min, [](size_t i) { return static_cast<double>(i); }}};
  Scores s = evaluate(objectives, n);
  REQUIRE(s.values.size() == n);
  size_t wrong = 0;
  for (size_t i = 0; i < n; ++i) {
    if (s.values[i] != static_cast<double>(i)) {
      ++wrong;
    }
  }
  CHECK(wrong == 0);
}

TEST_CASE("evaluate hands an objective's exception to the caller") {
  std::vector<Objective> objectives{
      {"fails at 777", Min, [](size_t i) -> double {
         if (i == 777) {
           throw std::runtime_error("bad candidate");
         }
         return 0;
       }}};
  CHECK_THROWS_AS(evaluate(objectives, 1000), std::runtime_error);
}

TEST_CASE("evaluate rejects missing objectives") {
  CHECK_THROWS_AS(evaluate(std::vector<Objective>{}, 3), std::invalid_argument);
  std::vector<Objective> no_function{Objective{.name = "no function"}};
  CHECK_THROWS_AS(evaluate(no_function, 3), std::invalid_argument);
}

TEST_CASE("end to end: three people on a line choose where to meet") {
  // People live at 0, 2 and 10. The candidates are the points 0, 1, ..., 10.
  //   x:      0   1   2   3   4   5   6  ...  10
  //   total: 12  11  10  11  12  13  14  ...  18
  //   worst: 10   9   8   7   6   5   6  ...  10
  // 2 has the best total, 5 the best worst case; 0, 1 and 6 onwards are
  // dominated.
  const V home{0, 2, 10};
  std::vector<Objective> objectives{
      {"total distance", Min,
       [&](size_t x) {
         double total = 0;
         for (double h : home) {
           total += std::abs(static_cast<double>(x) - h);
         }
         return total;
       }},
      {"longest trip", Min,
       [&](size_t x) {
         double worst = 0;
         for (double h : home) {
           worst = std::max(worst, std::abs(static_cast<double>(x) - h));
         }
         return worst;
       }},
  };
  Scores s = evaluate(objectives, 11);
  Ids front = pareto_front(s);
  CHECK(front == Ids{2, 3, 4, 5});
  CHECK(pick(s, front, V{1, 0}, Method::Chebyshev) == 2);
  CHECK(pick(s, front, V{0, 1}, Method::Chebyshev) == 5);
  CHECK(pick(s, front, V{2, 1}, Method::Chebyshev) == 3);
  CHECK(pick(s, front, V{1, 2}, Method::Chebyshev) == 4);
  // On a straight-line front the weighted sum only ever lands on an end.
  CHECK(pick(s, front, V{2, 1}, Method::WeightedSum) == 2);
  CHECK(pick(s, front, V{1, 2}, Method::WeightedSum) == 5);
}
