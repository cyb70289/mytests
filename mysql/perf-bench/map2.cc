#include <chrono>
#include <iostream>
#include <map>

constexpr int _map_size = 128;
constexpr int _loop_count = 8*1000000;

std::map<long, long> _map;

// key = 1, 2, ..., _map_size
void prepare_map() {
  for (long i = 0; i < _map_size; ++i) {
    _map[i+1] = i;
  }
}

int main(int argc, char* argv[]) {
  prepare_map();

  long s = 0;
  std::chrono::duration<double> elapsed{};
  for (int i = 0; i < _loop_count; ++i) {
    const auto t1 = std::chrono::high_resolution_clock::now();
    s += _map.upper_bound(i&1)->second;
    const auto t2 = std::chrono::high_resolution_clock::now();
    elapsed += (t2 - t1);
  }

  std::cout << elapsed.count()*1000000000/_loop_count << " ns\n";
  return s;
}
