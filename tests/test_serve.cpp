#include <cmath>
#include <doctest/doctest.h>
#include <stdexcept>
#include <string>
#include <themis/opt.hpp>
#include <themis/serve.hpp>
#include <thread>
#include <vector>

using namespace themis;
using nlohmann::json;

namespace {
// The three people on a line from test_opt.cpp, wrapped as a Model. People live
// at 0, 2 and 10, the candidates are the points 0 to 10, and the front is {2,
// 3, 4, 5}. Each point x is drawn at latitude 0, longitude x.
const std::vector<double> HOMES{0, 2, 10};

Model line_model() {
  std::vector<Objective> objectives{
      {"total distance", Direction::Min,
       [](size_t x) {
         double total = 0;
         for (double h : HOMES) {
           total += std::abs(static_cast<double>(x) - h);
         }
         return total;
       }},
      {"longest trip", Direction::Min,
       [](size_t x) {
         double worst = 0;
         for (double h : HOMES) {
           worst = std::max(worst, std::abs(static_cast<double>(x) - h));
         }
         return worst;
       }},
  };
  Model m;
  m.title = "three people on a line";
  m.objective_names = {"total distance", "longest trip"};
  m.scores = evaluate(objectives, 11);
  m.front = pareto_front(m.scores);
  for (size_t x = 0; x <= 10; ++x) {
    m.candidates.push_back(
        {"x" + std::to_string(x), 0, static_cast<double>(x)});
  }
  for (double h : HOMES) {
    m.entities.push_back({"home " + std::to_string(static_cast<int>(h)), 0, h});
  }
  m.detail = [](size_t x) {
    json trips = json::array();
    for (double h : HOMES) {
      trips.push_back(std::abs(static_cast<double>(x) - h));
    }
    return json{{"trips", trips}};
  };
  return m;
}
} // namespace

TEST_CASE("schema_json describes the objectives, the entities and the front") {
  json s = schema_json(line_model());
  CHECK(s["title"] == "three people on a line");
  CHECK(s["objectives"] ==
        json::parse(R"([{"name": "total distance", "direction": "min"},
                        {"name": "longest trip", "direction": "min"}])"));
  CHECK(s["entities"].size() == 3);
  CHECK(s["entities"][2] ==
        json::parse(R"({"label": "home 10", "lat": 0.0, "lon": 10.0})"));
  REQUIRE(s["front"].size() == 4);
  CHECK(s["front"][0] ==
        json::parse(R"({"candidate": 2, "label": "x2", "lat": 0.0, "lon": 2.0,
                        "scores": [10.0, 8.0]})"));
  CHECK(s["front"][3]["candidate"] == 5);
}

TEST_CASE("schema_json: a place without a position gets null coordinates") {
  Model m = line_model();
  m.candidates[3] = Place{"x3"};
  json s = schema_json(m);
  CHECK(s["front"][1]["label"] == "x3");
  CHECK(s["front"][1]["lat"].is_null());
  CHECK(s["front"][1]["lon"].is_null());
}

TEST_CASE("schema_json rejects an inconsistent model") {
  Model names = line_model();
  names.objective_names.pop_back();
  CHECK_THROWS_AS(schema_json(names), std::invalid_argument);
  Model places = line_model();
  places.candidates.pop_back();
  CHECK_THROWS_AS(schema_json(places), std::invalid_argument);
  Model empty = line_model();
  empty.front.clear();
  CHECK_THROWS_AS(schema_json(empty), std::invalid_argument);
  Model outside = line_model();
  outside.front.push_back(11);
  CHECK_THROWS_AS(schema_json(outside), std::invalid_argument);
}

TEST_CASE("solve_json picks with the requested weights and method") {
  Model m = line_model();
  // Chebyshev is the default. Weights (1, 2) pick 4, as in test_opt.cpp.
  json r = solve_json(m, json::parse(R"({"weights": [1, 2]})"));
  CHECK(r ==
        json::parse(R"({"candidate": 4, "label": "x4", "scores": [12.0, 6.0],
                             "detail": {"trips": [4.0, 2.0, 6.0]}})"));
  CHECK(
      solve_json(
          m,
          json::parse(
              R"({"weights": [1, 2], "method": "chebyshev"})"))["candidate"] ==
      4);
  CHECK(solve_json(
            m, json::parse(R"({"weights": [1, 2], "method": "weighted_sum"})"))
            ["candidate"] == 5);
  m.detail = nullptr; // no detail function: detail is null
  CHECK(
      solve_json(m, json::parse(R"({"weights": [1, 2]})"))["detail"].is_null());
}

TEST_CASE("solve_json rejects bad requests") {
  Model m = line_model();
  for (const char *body : {R"([1, 2])", R"({"w": [1, 2]})", R"({"weights": 3})",
                           R"({"weights": [1, "two"]})", R"({"weights": [1]})",
                           R"({"weights": [1, -2]})",
                           R"({"weights": [1, 2], "method": "best"})"}) {
    const std::string text = body;
    CAPTURE(text);
    CHECK_THROWS_AS(solve_json(m, json::parse(body)), std::invalid_argument);
  }
}

TEST_CASE("the server answers /schema and /solve over HTTP") {
  Model m = line_model();
  httplib::Server server;
  add_routes(server, m);
  const int port = server.bind_to_any_port("127.0.0.1");
  REQUIRE(port > 0);
  std::thread thread([&] { server.listen_after_bind(); });
  struct Stop { // stops the server even when a check below throws
    httplib::Server &server;
    std::thread &thread;
    ~Stop() {
      server.stop();
      thread.join();
    }
  } stop{server, thread};
  server.wait_until_ready();

  httplib::Client client("127.0.0.1", port);
  auto schema = client.Get("/schema");
  REQUIRE(schema);
  CHECK(schema->status == 200);
  CHECK(json::parse(schema->body) == schema_json(m));

  auto solve =
      client.Post("/solve", R"({"weights": [1, 2]})", "application/json");
  REQUIRE(solve);
  CHECK(solve->status == 200);
  CHECK(json::parse(solve->body)["label"] == "x4");

  auto bad_json = client.Post("/solve", "not json", "application/json");
  REQUIRE(bad_json);
  CHECK(bad_json->status == 400);
  CHECK(json::parse(bad_json->body).contains("error"));

  auto bad_weights =
      client.Post("/solve", R"({"weights": [1]})", "application/json");
  REQUIRE(bad_weights);
  CHECK(bad_weights->status == 400);
  CHECK(json::parse(bad_weights->body).contains("error"));
}
