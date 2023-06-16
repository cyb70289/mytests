#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <unistd.h>

#pragma GCC optimize ("align-functions=65536")

constexpr int64_t N = 256 * 1024;
constexpr int64_t L = 256 * 1024;

__attribute__((noinline))
uint32_t get5(const int64_t* index, const uint8_t* data, int64_t i) {
  return data[index[i]];
}

__attribute__((noinline))
uint32_t get4(const int64_t* index, const uint8_t* data, int64_t i) {
  return get5(index, data, i);
}

__attribute__((noinline))
uint32_t get3(const int64_t* index, const uint8_t* data, int64_t i) {
  return get4(index, data, i);
}

__attribute__((noinline))
uint32_t get2(const int64_t* index, const uint8_t* data, int64_t i) {
  return get3(index, data, i);
}

__attribute__((noinline))
uint32_t get1(const int64_t* index, const uint8_t* data, int64_t i) {
  return get2(index, data, i);
}

int main() {
  auto data = std::make_unique<uint8_t[]>(N);
  for (int64_t i = 0; i < N; ++i) {
    data[i] = i;
  }

  std::mt19937 mt(42);
  std::uniform_int_distribution<int64_t> dist(0, N-1);
  auto index = std::make_unique<int64_t[]>(L);
  for (int64_t i = 0; i < L; ++i) {
    index[i] = dist(mt);
  }

//  std::cout << "pid = " << getpid() << ", press enter to continue\n";
//  getchar();

  constexpr int n = 2000;

  uint32_t s = 0;
  auto t1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < n; ++i) {
    for (int64_t i = 0; i < L; ++i) {
      s += get2(index.get(), data.get(), i);
    }
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = t2 - t1;
  std::cout << "w/o icache miss: " << elapsed.count() << " s\n";

  t1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < n; ++i) {
    for (int64_t i = 0; i < L; ++i) {
      s += get1(index.get(), data.get(), i);
    }
  }
  t2 = std::chrono::high_resolution_clock::now();
  elapsed = t2 - t1;
  std::cout << "w/  icache miss: " << elapsed.count() << " s\n";

  return int(s);
}
