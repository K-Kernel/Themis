#pragma once
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

class NodeIds {
public:
  size_t id(std::string_view name) {
    auto it = index_.find(std::string(name));
    if (it != index_.end()) {
      return index_[std::string(name)];
    }
    size_t next = names_.size();
    index_[std::string(name)] = next;
    names_.emplace_back(name);
    return next;

  }; // its index; adds the name if new
  std::optional<size_t> find(std::string_view name) const {
    auto it = index_.find(std::string(name));
    if (it != index_.end()) {
      return index_.at(std::string(name));
    } else {
      return std::nullopt;
    }
  }; // its index, or std::nullopt
  const std::string &name(size_t index) const {
    return names_.at(index);
  }; // names_.at(index)
  size_t size() const { return names_.size(); };

private:
  std::unordered_map<std::string, size_t> index_{};
  std::vector<std::string> names_{};
};

struct Graph {
  std::vector<size_t> offsets{}, targets{}, sources{}, input_index{};

  size_t num_nodes() const {
    if (!offsets.empty()) {
      return offsets.size() - 1;
    }
    return 0;
  };

  size_t num_edges() const { return targets.size(); };

  template <class T> std::vector<T> permute(std::span<const T> by_input) const {
    if (by_input.size() != num_edges()) {
      throw std::invalid_argument("size doesn't match the number of edges");
    }
    std::vector<T> out(num_edges());
    for (size_t e = 0; e < num_edges(); ++e) {
      out[e] = by_input[input_index[e]];
    }
    return out;
  }
};

inline Graph build_graph(size_t num_nodes, std::span<const size_t> src,
                         std::span<const size_t> dst) {

  if (src.size() != dst.size()) {
    throw std::invalid_argument("src and dst don't match");
  }

  Graph graph{};
  graph.offsets = std::vector<size_t>(num_nodes + 1, 0);
  graph.targets.resize(src.size());
  graph.sources.resize(src.size());
  graph.input_index.resize(src.size());

  for (size_t i{0}; i < src.size(); ++i) {
    if (src[i] >= num_nodes || dst[i] >= num_nodes) {
      throw std::out_of_range("node index out of range");
    }
    ++graph.offsets[src[i] + 1];
  }
  for (size_t u{0}; u < num_nodes; ++u) {
    graph.offsets[u + 1] += graph.offsets[u];
  }

  std::vector<size_t> next = graph.offsets;
  next.pop_back();

  for (size_t i{0}; i < src.size(); ++i) {
    size_t e = next[src[i]]++;
    graph.targets[e] = dst[i];
    graph.sources[e] = src[i];
    graph.input_index[e] = i;
  }

  return graph;
}
} // namespace themis
