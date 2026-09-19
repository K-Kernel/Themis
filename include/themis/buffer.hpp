#pragma once
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

struct Buffer {
  std::span<const char> buffer_view;
  std::shared_ptr<void> file_buffer;
};

inline Buffer read_csv(std::string path) {
  std::ifstream csv(path, std::ios_base::binary);

  if (!csv) {
    std::runtime_error("couldn't open the file : " + path);
  }

  csv.seekg(0, std::ios::end);
  auto csv_size = csv.tellg();

  csv.seekg(0);
  auto storage = std::make_shared<std::vector<char>>(csv_size);

  csv.read(storage->data(), storage->size());

  return Buffer{std::span<const char>(*storage), storage};
}
