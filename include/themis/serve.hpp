#pragma once
#include <nlohmann/json_fwd.hpp>
#include <span>
#include <themis/opt.hpp>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Skeleton for include/themis/serve.hpp. Every body below throws "not written
// yet". Replace them in step order, uncommenting each parameter name as you go,
// and delete this note when all of them are written.
//
// The server answers two requests. The browser shell (next week) reads the
// schema once and builds its sliders and charts from it, then sends a solve
// request every time a slider moves:
//   GET  /schema  -> {"title", "objectives": [{"name", "direction"}],
//                     "entities": [{"label", "lat", "lon"}],
//                     "front": [{"candidate", "label", "lat", "lon",
//                     "scores"}]}
//   POST /solve   {"weights": [...], "method": "chebyshev" or "weighted_sum"}
//                 -> {"candidate", "label", "scores", "detail"}
// A bad solve request gets status 400 and {"error": "..."}.

namespace themis {

// Something with a name and, if it has one, a position on the map.
struct Place {
  std::string label{};
  double lat{std::numeric_limits<double>::quiet_NaN()};
  double lon{std::numeric_limits<double>::quiet_NaN()};
};

// Everything the server shows. Built once, then only read, so the server's
// threads can share it without locking.
struct Model {
  std::string title{};
  std::vector<std::string> objective_names{}; // one per objective
  Scores scores{};                            // from evaluate()
  std::vector<size_t> front{};                // from pareto_front(scores)
  std::vector<Place> candidates{};            // one per candidate
  std::vector<Place> entities{};              // drawn as they are, e.g. offices
  std::function<nlohmann::json(size_t)>
      detail{}; // optional breakdown of a pick
};

namespace detail {
// Step 1. std::invalid_argument when objective_names or candidates do not match
// the size of scores, when the front is empty, or when a front member is not a
// candidate index.
inline void check_model(const Model &m) {
  if (m.objective_names.size() != m.scores.num_objectives() ||
      m.candidates.size() != m.scores.num_candidates() || m.front.empty()) {
    throw std::invalid_argument("Model is invalid");
  }

  for (size_t i : m.front) {
    if (i >= m.candidates.size()) {
      throw std::invalid_argument("index is bigger than candidates");
    }
  }
}

// Step 1. {"label": ..., "lat": ..., "lon": ...}, with null in place of a NaN
// coordinate: nlohmann::json(nullptr). (nlohmann prints a NaN as null, but the
// json value itself would still hold a number, and the tests look at the
// value.)
inline nlohmann::json place_json(const Place &p) {

  if (std::isnan(p.lat) && std::isnan(p.lon)) {
    return nlohmann::json{{"label", p.label},
                          {"lat", nlohmann::json(nullptr)},
                          {"lon", nlohmann::json(nullptr)}};
  }

  nlohmann::json place{{"label", p.label}, {"lat", p.lat}, {"lon", p.lon}};
  return place;
}
} // namespace detail

// Step 1. The /schema answer. Calls detail::check_model first. Objectives in
// order, "direction" is "min" or "max". One front entry per front member, in
// front order: place_json of its candidate plus "candidate" (the index) and
// "scores" (its row as an array).
//   nlohmann::json out;  out["title"] = m.title;  out["front"] =
//   nlohmann::json::array(); out["front"].push_back(entry);   {{"name", n},
//   {"direction", "min"}} is an object; a std::vector<double> converts to an
//   array by assignment.
inline nlohmann::json schema_json(const Model &m) {
  detail::check_model(m);
  nlohmann::json out{};

  out["title"] = m.title;
  out["objectives"] = nlohmann::json::array();
  out["entities"] = nlohmann::json::array();
  out["front"] = nlohmann::json::array();

  for (size_t k{0}; k < m.objective_names.size(); ++k) {

    std::string direction =
        (m.scores.directions[k] == Direction::Max) ? "max" : "min";
    out["objectives"].push_back(
        {{"name", m.objective_names[k]}, {"direction", direction}});
  };

  for (const Place &e : m.entities) {
    out["entities"].push_back(detail::place_json(e));
  }

  for (size_t c : m.front) {
    nlohmann::json item = detail::place_json(m.candidates[c]);
    std::span<const double> front_row{m.scores.row(c)};

    item["candidate"] = c;
    item["scores"] = std::vector<double>(front_row.begin(), front_row.end());

    out["front"].push_back(item);
  }

  return out;
}

// Step 2. The /solve answer for a parsed request body. std::invalid_argument
// when the body is not an object, has no "weights" array, holds a weight that
// is not a number, or names a method other than "chebyshev" or "weighted_sum".
// "method" is optional and defaults to "chebyshev". Bad weight values are left
// to pick(), which already throws std::invalid_argument for them. "detail" is
// m.detail(pick) when m.detail is set, otherwise null.
//   request.is_object(), request.contains("weights"),
//   request["weights"].is_array(), for (const nlohmann::json &w :
//   request["weights"]), w.is_number(), w.get<double>()
inline nlohmann::json solve_json(const Model &m,
                                 const nlohmann::json &request) {

  if (!request.is_object() || !request.contains("weights") ||
      !request["weights"].is_array()) {
    throw std::invalid_argument(
        "request is not an object, or incorrect weights");
  }

  std::vector<double> weigths;
  for (const auto &w : request["weights"]) {
    if (!w.is_number()) {
      throw std::invalid_argument("weight is not a number");
    } else {
      weigths.push_back(w.get<double>());
    }
  }

  Method method = Method::Chebyshev;
  if (request.contains("method")) {
    if (request["method"] == "chebyshev") {
    } else if (request["method"] == "weighted_sum") {
      method = Method::WeightedSum;
    } else {
      throw std::invalid_argument("wrong method");
    }
  }

  size_t c = pick(m.scores, m.front, weigths, method);
  std::span<const double> row(m.scores.row(c));

  return nlohmann::json{
      {"candidate", c},
      {"label", m.candidates[c].label},
      {"scores", std::vector<double>(row.begin(), row.end())},
      {"detail", m.detail ? m.detail(c) : nlohmann::json(nullptr)}};
}

// Step 3. Registers the two routes on a server. Build the schema text once here
// (schema_json(m).dump()) and capture it in the GET handler. The POST handler
// parses req.body with nlohmann::json::parse and answers
// solve_json(...).dump(). A nlohmann::json::exception (bad JSON) or a
// std::invalid_argument (bad request) becomes res.status = 400 with {"error":
// e.what()}.
//   server.Get("/schema", [schema](const httplib::Request &, httplib::Response
//   &res) {
//     res.set_content(schema, "application/json");
//   });
// The handlers capture m by reference, so m must outlive the server.
inline void add_routes(httplib::Server &server, const Model &m) {
  std::string schema{schema_json(m).dump()};

  server.Get("/schema",
             [schema](const httplib::Request &, httplib::Response &res) {
               res.set_content(schema, "application/json");
             });

  server.Post(
      "/solve", [m](const httplib::Request &req, httplib::Response &res) {
        try {
          nlohmann::json req_json = nlohmann::json::parse(req.body);
          res.set_content(solve_json(m, req_json).dump(), "application/json");
        } catch (nlohmann::json::exception &e) {
          res.status = 400;
          res.set_content(nlohmann::json{{"error", e.what()}}.dump(),
                          "application/json");
        } catch (std::invalid_argument &e) {
          res.status = 400;
          res.set_content(nlohmann::json{{"error", e.what()}}.dump(),
                          "application/json");
        }
      });
}

// Step 4. Creates an httplib::Server, adds the routes, serves the files in
// shell_dir at "/" (server.set_mount_point("/", shell_dir) returns false when
// the folder is missing: print a warning and carry on), prints the address and
// blocks in server.listen(host, port). std::runtime_error when listen returns
// false, which means the port is taken.

inline void serve(const Model &m, int port,
                  const std::string &shell_dir = "shell",
                  const std::string &host = "127.0.0.1") {
  httplib::Server svr;
  // httplib defaults to SO_REUSEPORT, which lets a second server bind the same
  // port and share the requests. SO_REUSEADDR alone makes the second bind fail
  // instead.
  svr.set_socket_options([](socket_t sock) {
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  });
  add_routes(svr, m);
  if (!svr.set_mount_point("/", shell_dir)) {
    std::fprintf(stderr,
                 "serve: no folder '%s', serving only /schema and /solve\n",
                 shell_dir.c_str());
  }
  if (!svr.bind_to_port(host, port)) {
    throw std::runtime_error("serve: cannot listen on port " +
                             std::to_string(port) +
                             " (another server running?)");
  }
  std::printf("serving on http://%s:%d\n", host.c_str(), port);
  std::fflush(stdout);
  svr.listen_after_bind();
}
} // namespace themis
