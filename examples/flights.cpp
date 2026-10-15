// themis-flights airports.dat routes.dat FROM TO
// Shortest flown distance between two airports on the OpenFlights route network.
#include <themis/geo.hpp>
#include <themis/graph.hpp>
#include <themis/io.hpp>
#include <themis/search.hpp>

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
static double us_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::micro>(Clock::now() - t0).count();
}

int main(int argc, char **argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: themis-flights airports.dat routes.dat FROM TO\n");
    return 2;
  }
  themis::Table airports = themis::load_csv(argv[1], false);
  themis::Table routes = themis::load_csv(argv[2], false);

  // 1. Nodes: every airport with an IATA code (column 4), at (lat, lon) = columns 6, 7.
  themis::NodeIds ids;
  std::vector<double> lat, lon;
  auto codes = airports.strings(4);
  auto lats = airports.doubles(6);
  auto lons = airports.doubles(7);
  for (size_t r = 0; r < airports.nrows(); ++r) {
    if (!airports.column(4).valid[r]) continue;
    size_t before = ids.size();
    size_t v = ids.id(codes[r]);
    if (v == before) {  // a new node: remember where it is
      lat.push_back(lats[r]);
      lon.push_back(lons[r]);
    }
  }

  // 2. Edges: every route whose two airport codes (columns 2 and 4) are known.
  std::vector<size_t> src, dst;
  std::vector<double> km;
  auto from = routes.strings(2);
  auto to = routes.strings(4);
  size_t skipped = 0;
  for (size_t r = 0; r < routes.nrows(); ++r) {
    auto a = ids.find(from[r]);
    auto b = ids.find(to[r]);
    if (!a || !b) {
      ++skipped;
      continue;
    }
    src.push_back(*a);
    dst.push_back(*b);
    km.push_back(themis::haversine_km(lat[*a], lon[*a], lat[*b], lon[*b]));
  }
  themis::Graph g = themis::build_graph(ids.size(), src, dst);
  std::vector<double> weight = g.permute<double>(km);
  std::printf("graph: %zu airports, %zu routes (%zu skipped: unknown airport code)\n",
              g.num_nodes(), g.num_edges(), skipped);

  // 3. Query.
  auto s = ids.find(argv[3]);
  auto t = ids.find(argv[4]);
  if (!s || !t) {
    std::fprintf(stderr, "unknown airport code\n");
    return 1;
  }
  std::vector<double> h(g.num_nodes());  // straight-line km to the target: never more than flown km
  for (size_t v = 0; v < g.num_nodes(); ++v)
    h[v] = themis::haversine_km(lat[v], lon[v], lat[*t], lon[*t]);

  auto t0 = Clock::now();
  themis::ShortestPaths d = themis::dijkstra(g, weight, *s, *t);
  double d_us = us_since(t0);
  t0 = Clock::now();
  themis::ShortestPaths a = themis::astar(g, weight, *s, *t, h);
  double a_us = us_since(t0);

  if (a.dist[*t] == themis::UNREACHABLE) {
    std::printf("%s -> %s: no route\n", argv[3], argv[4]);
    return 0;
  }
  std::string path = ids.name(*s);
  for (size_t e : themis::path_edges(g, a, *t)) path += " -> " + ids.name(g.targets[e]);
  std::printf("%s\n%.0f km, %zu flights\n", path.c_str(), a.dist[*t],
              themis::path_edges(g, a, *t).size());
  std::printf("dijkstra: %zu settled, %.0f us\nastar:    %zu settled, %.0f us\n",
              d.settled, d_us, a.settled, a_us);
}
