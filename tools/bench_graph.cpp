#include <themis/graph.hpp>
#include <themis/search.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

int main() {
  const size_t W = 316;
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
  std::mt19937_64 rng(42);
  std::uniform_real_distribution<double> uniform(1.0, 10.0);
  std::vector<double> w_in(src.size());
  for (double &w : w_in)
    w = uniform(rng);

  using Clock = std::chrono::steady_clock;
  auto t0 = Clock::now();
  themis::Graph g = themis::build_graph(W * W, src, dst);
  std::vector<double> w = g.permute<double>(w_in);
  double build_ms =
      std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

  double best_ms = 1e300, far = 0;
  for (int run = 0; run < 5; ++run) {
    t0 = Clock::now();
    themis::ShortestPaths sp = themis::dijkstra(g, w, 0);
    best_ms = std::min(
        best_ms,
        std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
    far = sp.dist[g.num_nodes() - 1];
  }
  std::printf(
      "%zu nodes, %zu edges\nbuild %.1f ms\ndijkstra (best of 5) %.1f ms\n"
      "distance to the far corner %.4f\n",
      g.num_nodes(), g.num_edges(), build_ms, best_ms, far);

  return 0;
}
