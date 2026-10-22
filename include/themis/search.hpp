#pragma once
#include "themis/graph.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>
namespace themis {

inline constexpr size_t NO_EDGE =
    std::numeric_limits<size_t>::max(); // "no parent edge"
inline constexpr size_t NO_TARGET =
    std::numeric_limits<size_t>::max(); // "search everything"
inline constexpr double UNREACHABLE = std::numeric_limits<double>::infinity();
struct ShortestPaths {
  std::vector<double> dist{}; // UNREACHABLE where there's no path
  std::vector<size_t>
      parent_edge{}; // NO_EDGE for the source and unreachable nodes
  size_t settled{0}; // nodes finalised: shows how much less work A* does
};

inline ShortestPaths astar(const Graph &g, std::span<const double> weight,
                           size_t source, size_t target,
                           std::span<const double> heuristic) {
  if (weight.size() != g.num_edges()) {
    throw std::invalid_argument("wrong number of edges");
  }

  if (heuristic.size() != g.num_nodes()) {
    throw std::invalid_argument("Inavlid heuristic");
  }

  if (source >= g.num_nodes() ||
      (target != NO_TARGET && target >= g.num_nodes())) {
    throw std::out_of_range("source its bigger than number of nodes");
  }

  for (double w : weight) {
    if (!(w >= 0)) {
      throw std::invalid_argument(" weight must be >= 0");
    }
  }
  ShortestPaths shortest_path{std::vector<double>(g.num_nodes(), UNREACHABLE),
                              std::vector<size_t>(g.num_nodes(), NO_EDGE)};
  std::vector<char> done(g.num_nodes(), 0);

  std::priority_queue<std::pair<double, size_t>,
                      std::vector<std::pair<double, size_t>>, std::greater<>>
      mini_heap;
  shortest_path.dist[source] = 0;
  mini_heap.push({heuristic[source], source});

  while (!mini_heap.empty()) {
    std::pair<double, size_t> u = mini_heap.top();
    mini_heap.pop();

    if (done[u.second]) {
      continue;
    }

    done[u.second] = 1;
    shortest_path.settled++;

    if (u.second == target && target != NO_TARGET) {
      break;
    }

    for (size_t e = g.offsets[u.second]; e < g.offsets[u.second + 1]; ++e) {
      size_t v = g.targets[e];
      double nd = shortest_path.dist[u.second] + weight[e];

      if (nd < shortest_path.dist[v]) {
        shortest_path.dist[v] = nd;
        shortest_path.parent_edge[v] = e;
        mini_heap.push({nd + heuristic[v], v});
      }
    }
  }

  return shortest_path;
};

inline ShortestPaths dijkstra(const Graph &g, std::span<const double> weight,
                              size_t source, size_t target = NO_TARGET) {
  std::vector<double> heuristic(g.num_nodes(), 0);
  return astar(g, weight, source, target, heuristic);
};

inline std::vector<size_t> path_edges(const Graph &g, const ShortestPaths &sp,
                                      size_t target) {

  if (sp.dist[target] == UNREACHABLE) {
    return {};
  } else {

    std::vector<size_t> path{};
    size_t v = target;
    while (sp.parent_edge[v] != NO_EDGE) {
      size_t e = sp.parent_edge[v];
      path.push_back(e); // the edge into v
      v = g.sources[e];  // step back to where that edge starts
    }
    std::reverse(path.begin(), path.end());
    return path;
  }
}
} // namespace themis
