#include <chrono>
#include <iostream>

constexpr int _loop_count = 8*1000000;

int main(int argc, char* argv[]) {
  std::chrono::duration<double> elapsed{};
  for (int i = 0; i < _loop_count; ++i) {
    const auto t1 = std::chrono::high_resolution_clock::now();
    const auto t2 = std::chrono::high_resolution_clock::now();
    elapsed += (t2 - t1);
  }

  const auto overhead = elapsed.count()*1000000000/_loop_count/2;
  std::cout << "overhead of chrono::now(): " << overhead << " ns\n";

  return 0;
}
