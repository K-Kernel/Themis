// themis-meet-serve airports.dat routes.dat CODE CODE [CODE ...]
// The model from meet.cpp behind a web server on http://127.0.0.1:8080. Run it from the
// repository root so it finds the shell/ folder. Then, from another terminal:
//   curl -s http://127.0.0.1:8080/schema
//   curl -s -X POST http://127.0.0.1:8080/solve -d '{"weights": [1, 1, 1]}'
#include <themis/geo.hpp>
#include <themis/graph.hpp>
#include <themis/io.hpp>
#include <themis/opt.hpp>
#include <themis/search.hpp>
#include <themis/serve.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using Clock = std::chrono::steady_clock;
static double ms_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

int main(int argc, char **argv) {
  if (argc < 5) {
    std::fprintf(stderr, "usage: themis-meet-serve airports.dat routes.dat CODE CODE [CODE ...]\n");
    return 2;
  }
  themis::Table airports = themis::load_csv(argv[1], false);
  themis::Table routes = themis::load_csv(argv[2], false);

  // 1. The flight graph, built exactly as in flights.cpp.
  themis::NodeIds ids;
  std::vector<double> lat, lon;
  auto codes = airports.strings(4);
  auto lats = airports.doubles(6);
  auto lons = airports.doubles(7);
  for (size_t r = 0; r < airports.nrows(); ++r) {
    if (!airports.column(4).valid[r]) continue;
    size_t before = ids.size();
    size_t v = ids.id(codes[r]);
    if (v == before) {
      lat.push_back(lats[r]);
      lon.push_back(lons[r]);
    }
  }
  std::vector<size_t> src, dst;
  std::vector<double> km;
  auto from = routes.strings(2);
  auto to = routes.strings(4);
  for (size_t r = 0; r < routes.nrows(); ++r) {
    auto a = ids.find(from[r]);
    auto b = ids.find(to[r]);
    if (!a || !b) continue;
    src.push_back(*a);
    dst.push_back(*b);
    km.push_back(themis::haversine_km(lat[*a], lon[*a], lat[*b], lon[*b]));
  }
  themis::Graph g = themis::build_graph(ids.size(), src, dst);
  std::vector<double> weight = g.permute<double>(km);
  const size_t n = g.num_nodes();

  // 2. One search per traveller: km and flights from their airport to every airport.
  std::vector<size_t> home;
  for (int i = 3; i < argc; ++i) {
    auto v = ids.find(argv[i]);
    if (!v) {
      std::fprintf(stderr, "unknown airport code %s\n", argv[i]);
      return 1;
    }
    home.push_back(*v);
  }
  const size_t people = home.size();
  auto t0 = Clock::now();
  std::vector<std::vector<double>> km_to(people), flights_to(people);
  std::vector<themis::ShortestPaths> trees(people);
  for (size_t p = 0; p < people; ++p) {
    trees[p] = themis::dijkstra(g, weight, home[p]);
    km_to[p] = trees[p].dist;
    flights_to[p].resize(n);
    for (size_t v = 0; v < n; ++v) {
      flights_to[p][v] = trees[p].dist[v] == themis::UNREACHABLE
                             ? themis::UNREACHABLE
                             : static_cast<double>(themis::path_edges(g, trees[p], v).size());
    }
  }
  double search_ms = ms_since(t0);

  // 3. The objectives. Each one reads the tables above and nothing else, so it is safe to
  //    call from many threads at once.
  std::vector<themis::Objective> objectives{
      {"total km", themis::Direction::Min,
       [&](size_t c) {
         double total = 0;
         for (size_t p = 0; p < people; ++p) total += km_to[p][c];
         return total;
       }},
      {"worst km", themis::Direction::Min,
       [&](size_t c) {
         double worst = 0;
         for (size_t p = 0; p < people; ++p) worst = std::max(worst, km_to[p][c]);
         return worst;
       }},
      {"flights", themis::Direction::Min,
       [&](size_t c) {
         double total = 0;
         for (size_t p = 0; p < people; ++p) total += flights_to[p][c];
         return total;
       }},
  };

  t0 = Clock::now();
  themis::Scores scores = themis::evaluate(objectives, n);
  double eval_ms = ms_since(t0);
  t0 = Clock::now();
  std::vector<size_t> front = themis::pareto_front(scores);
  double front_ms = ms_since(t0);

  size_t reachable = 0;
  for (size_t c = 0; c < n; ++c) {
    if (scores.row(c)[0] != themis::UNREACHABLE) ++reachable;
  }
  std::printf("%zu travellers, %zu candidate airports, %zu reachable by everyone\n", people, n,
              reachable);
  std::printf("pareto front: %zu airports\n", front.size());

  // 4. The model: what the browser sees. Front members and travellers are drawn by position.
  themis::Model model;
  model.title = "meeting point for " + std::to_string(people) + " travellers";
  for (const themis::Objective &o : objectives) model.objective_names.push_back(o.name);
  model.scores = scores;
  model.front = front;
  for (size_t v = 0; v < n; ++v) model.candidates.push_back({ids.name(v), lat[v], lon[v]});
  for (size_t h : home) model.entities.push_back({ids.name(h), lat[h], lon[h]});
  model.detail = [&](size_t c) {
    nlohmann::json trips = nlohmann::json::array();
    for (size_t p = 0; p < people; ++p) {
      nlohmann::json route = nlohmann::json::array({ids.name(home[p])});
      for (size_t e : themis::path_edges(g, trees[p], c)) route.push_back(ids.name(g.targets[e]));
      trips.push_back({{"route", route}, {"km", km_to[p][c]}, {"flights", flights_to[p][c]}});
    }
    return nlohmann::json{{"trips", trips}};
  };
  std::printf("search %.1f ms, evaluate %.2f ms, front %.2f ms\n", search_ms, eval_ms, front_ms);

  themis::serve(model, 8080);
  return 0;
}
