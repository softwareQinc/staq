#define FMT_HEADER_ONLY = true

#include "mapping/device.hpp"

#include <list>
#include <iostream>

using namespace synthewareQ::mapping;

void show_path(const path& p) {
  std::cout << "  ";
  for (auto it = p.begin(); it != p.end(); it++) {
    if (it != p.begin()) std::cout << "-->";
    std::cout << *it;
  }
  std::cout << "\n";
}

void show_tree(const spanning_tree& tree) {
  std::cout << "  ";
  for (auto [i, j] : tree) {
    std::cout << "(" << i << "," << j << ")";
  }
  std::cout << "\n";
}

int main(int argc, char** argv) {

  std::cout << "Shortest path between 0 & 3:\n";
  show_path(square_9q.shortest_path(0, 3));
  std::cout << "Shortest path between 2 & 8:\n";
  show_path(square_9q.shortest_path(2, 8));
  std::cout << "Shortest path between 1 & 5:\n";
  show_path(square_9q.shortest_path(1, 5));
  std::cout << "Shortest path between 8 & 0:\n";
  show_path(square_9q.shortest_path(8, 0));

  std::cout << "\n";

  std::cout << "Steiner tree {0, 2, 6}:\n";
  show_tree(square_9q.steiner({2, 6}, 0));
  std::cout << "Steiner tree {1, 3, 8}:\n";
  show_tree(square_9q.steiner({3, 8}, 1));
  std::cout << "Steiner tree {0, 2, 7}:\n";
  show_tree(square_9q.steiner({2, 7}, 0));
  std::cout << "Steiner tree {1, 5, 3, 8}:\n";
  show_tree(square_9q.steiner({5, 3, 8}, 1));
  std::cout << "Steiner tree {0, 1, 2, 3, 4, 5, 6, 7, 8}:\n";
  show_tree(square_9q.steiner({1, 2, 3, 4, 5, 6, 7, 8}, 0));

  return 1;
}
