#include <cmath>
#include <doctest/doctest.h>
#include <themis/geo.hpp>
#include <themis/graph.hpp>
#include <themis/search.hpp>
#include <vector>

using namespace themis;

namespace {
// The paper exercise: 4 nodes, 6 edges, given out of order.
//   input edge:  0     1     2     3     4     5
//   src -> dst: 2->3  0->1  1->2  0->2  2->0  1->3
//   km:          4     1     2     5     1     7
const std::vector<size_t> SRC{2, 0, 1, 0, 2, 1};
const std::vector<size_t> DST{3, 1, 2, 2, 0, 3};
const std::vector<double> KM{4, 1, 2, 5, 1, 7};
} // namespace

TEST_CASE("CSR layout of the paper graph") {
  Graph g = build_graph(4, SRC, DST);
  CHECK(g.num_nodes() == 4);
  CHECK(g.num_edges() == 6);
  CHECK(g.offsets == std::vector<size_t>{0, 2, 4, 6, 6});
  CHECK(g.targets == std::vector<size_t>{1, 2, 2, 3, 3, 0});
  CHECK(g.sources == std::vector<size_t>{0, 0, 1, 1, 2, 2});
  CHECK(g.input_index == std::vector<size_t>{1, 3, 2, 5, 0, 4});
}

TEST_CASE("permute puts attributes in edge-id order") {
  Graph g = build_graph(4, SRC, DST);
  CHECK(g.permute<double>(KM) == std::vector<double>{1, 5, 2, 7, 4, 1});
  CHECK_THROWS_AS(g.permute<double>(std::vector<double>{1, 2}),
                  std::invalid_argument);
}

TEST_CASE("build_graph rejects bad input") {
  CHECK_THROWS_AS(
      build_graph(2, std::vector<size_t>{0, 1}, std::vector<size_t>{1}),
      std::invalid_argument);
  CHECK_THROWS_AS(
      build_graph(2, std::vector<size_t>{0}, std::vector<size_t>{5}),
      std::out_of_range);
}

TEST_CASE("NodeIds") {
  NodeIds ids;
  CHECK(ids.id("LHR") == 0);
  CHECK(ids.id("JFK") == 1);
  CHECK(ids.id("LHR") == 0);
  CHECK(ids.size() == 2);
  CHECK(ids.name(1) == "JFK");
  CHECK(ids.find("JFK") == 1);
  CHECK_FALSE(ids.find("SYD").has_value());
}

TEST_CASE("Dijkstra takes the cheap detours") {
  Graph g = build_graph(4, SRC, DST);
  std::vector<double> w = g.permute<double>(KM);
  ShortestPaths sp = dijkstra(g, w, 0);
  CHECK(sp.dist == std::vector<double>{0, 1, 3, 7});
  CHECK(path_edges(g, sp, 3) ==
        std::vector<size_t>{0, 2, 4}); // 0->1->2->3, not 0->2 or 1->3
  CHECK(path_edges(g, sp, 0).empty());
}

TEST_CASE("Unreachable nodes") {
  Graph g = build_graph(4, SRC, DST);
  std::vector<double> w = g.permute<double>(KM);
  ShortestPaths sp = dijkstra(g, w, 3); // node 3 has no out-edges
  CHECK(sp.dist[3] == 0);
  CHECK(sp.dist[0] == UNREACHABLE);
  CHECK(path_edges(g, sp, 0).empty());
}

TEST_CASE("Parallel edges: the cheaper one wins") {
  Graph g =
      build_graph(2, std::vector<size_t>{0, 0}, std::vector<size_t>{1, 1});
  ShortestPaths sp = dijkstra(g, std::vector<double>{5, 2}, 0);
  CHECK(sp.dist[1] == 2);
  CHECK(sp.parent_edge[1] == 1);
}

TEST_CASE("Search rejects bad input") {
  Graph g = build_graph(4, SRC, DST);
  CHECK_THROWS_AS(dijkstra(g, std::vector<double>{1, 2}, 0),
                  std::invalid_argument);
  CHECK_THROWS_AS(dijkstra(g, std::vector<double>{1, 1, 1, -1, 1, 1}, 0),
                  std::invalid_argument);
  CHECK_THROWS_AS(dijkstra(g, std::vector<double>(6, 1.0), 9),
                  std::out_of_range);
}

TEST_CASE("A* matches Dijkstra and settles fewer nodes") {
  // 30 x 30 grid, unit steps in 4 directions; heuristic = straight-line
  // distance.
  const size_t W = 30;
  std::vector<size_t> src, dst;
  auto node = [&](size_t x, size_t y) { return y * W + x; };
  for (size_t y = 0; y < W; ++y)
    for (size_t x = 0; x < W; ++x) {
      if (x + 1 < W) {
        src.push_back(node(x, y));
        dst.push_back(node(x + 1, y));
        src.push_back(node(x + 1, y));
        dst.push_back(node(x, y));
      }
      if (y + 1 < W) {
        src.push_back(node(x, y));
        dst.push_back(node(x, y + 1));
        src.push_back(node(x, y + 1));
        dst.push_back(node(x, y));
      }
    }
  Graph g = build_graph(W * W, src, dst);
  std::vector<double> w(g.num_edges(), 1.0);
  size_t s = node(0, W / 2),
         t = node(W - 1, W / 2); // straight across the middle
  std::vector<double> h(W * W);
  for (size_t y = 0; y < W; ++y)
    for (size_t x = 0; x < W; ++x)
      h[node(x, y)] = std::hypot(double(W - 1 - x), double(y) - double(W / 2));
  ShortestPaths d = dijkstra(g, w, s, t);
  ShortestPaths a = astar(g, w, s, t, h);
  CHECK(a.dist[t] == d.dist[t]);
  CHECK(a.dist[t] == double(W - 1));
  MESSAGE("settled: dijkstra ", d.settled, ", astar ", a.settled);
  CHECK(a.settled < d.settled);
}

TEST_CASE("haversine") {
  CHECK(haversine_km(51.4775, -0.461389, 51.4775, -0.461389) == 0);
  CHECK(std::abs(haversine_km(0, 0, 0, 1) - 111.195) <
        0.01); // one degree on the equator
}
