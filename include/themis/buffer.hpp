#pragma once
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

struct Buffer {
  std::span<const char> buffer_view;
  std::shared_ptr<void> file_buffer;
};

inline Buffer read_data(std::string path) {
  std::ifstream dataset(path, std::ios_base::binary);

  dataset.seekg(0, std::ios::end);
  auto dataset_size = dataset.tellg();

  dataset.seekg(0);
  auto storage = std::make_shared<std::vector<char>>(dataset_size);

  dataset.read(storage->data(), storage->size());

  return Buffer{std::span<const char>(*storage), storage};
}
