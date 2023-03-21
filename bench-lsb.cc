// ubuntu: g++ -O2 -std=c++11 -o bench-lsb bench-lsb.cc -lbenchmark
// mac m1: g++ -O2 -std=c++11 -I/opt/homebrew/include -L/opt/homebrew/lib -o bench-lsb bench-lsb.cc -lbenchmark

#include <stdio.h>
#include <string.h>

#include <random>
#include <vector>
#include <benchmark/benchmark.h>

constexpr int _size = 1000;
std::vector<size_t> _data;

static inline size_t test_ffsll(size_t x) {
  // gcc refuses to inline ffsll(), use builtin instead
  return __builtin_ffsll(x) - 1;
}

// x != 0
static inline size_t test_optim(size_t x) {
  // size_t idx;
  // asm("rbit %0,%1" : "=r"(idx) : "r"(x));
  // asm("clz  %0,%1" : "=r"(idx) : "r"(idx));
  // return idx;
  return __builtin_ctzll(x);
}

static void BM_ffsll(benchmark::State& state) {
  for (auto _ : state) {
    size_t s = 0;
    for (const auto d : _data) {
      s += test_ffsll(d);
    }
    benchmark::DoNotOptimize(s);
  }
}

static void BM_optim(benchmark::State& state) {
  for (auto _ : state) {
    size_t s = 0;
    for (const auto d : _data) {
      s += test_optim(d);
    }
    benchmark::DoNotOptimize(s);
  }
}

BENCHMARK(BM_ffsll);
BENCHMARK(BM_optim);

int main(int argc, char* argv[]) {
  std::mt19937 mt(42);
  std::uniform_int_distribution<size_t> dist(1, ~0ULL);
  _data.reserve(_size);
  for (int i = 0; i < _size; ++i) {
    _data.push_back(dist(mt));
  }

  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}
