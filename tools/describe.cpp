
#include "themis/io.hpp"
#include "themis/table.hpp"
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: ./file [--no-header] input.csv" << std::endl;
    return 2;
  }
  std::string option{argv[1]};

  if (option == "--no-header") {
    themis::Table t = themis::load_csv(argv[2]);
    std::cout << t.nrows() << " rows" << " x" << t.ncols() << " columns"
              << std::endl;

    std::unordered_map<int, int> freq;
    for (themis::Table::parse_error errors : t.errors) {
      ++freq[errors.kind];
    }
    std::cout << "Errors: " << t.errors.size() << std::endl;
    std::cout << "Short Rows: " << freq[0] << std::endl;
    std::cout << "Long Rows: " << freq[1] << std::endl;
    std::cout << "Untermianted Rows: " << freq[2] << std::endl;

  } else if (option == "--header") {
    themis::Table t = themis::load_csv(argv[2], true);

    std::unordered_map<int, int> freq;
    for (themis::Table::parse_error errors : t.errors) {
      ++freq[errors.kind];
    }
    std::cout << "Errors: " << t.errors.size();
    std::cout << "Short Rows" << freq[0];
    std::cout << "Long Rows" << freq[1];
    std::cout << "Untermianted Rows" << freq[2];
  }
}
