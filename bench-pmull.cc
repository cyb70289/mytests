// g++ -O2 -std=c++11 -o bench-pmull bench-pmull.cc -lbenchmark -march=native

#include <stdio.h>
#include <string.h>

#include <arm_acle.h>
#include <arm_neon.h>

#include <random>
#include <vector>
#include <benchmark/benchmark.h>

static void BM_u32(benchmark::State& state) {
  uint32_t u32 = 1;
  for (auto _ : state) {
    for (int i = 0; i < 100; ++i) {
      u32 = vmull_p64(u32, 0x01234567);
    }
    benchmark::DoNotOptimize(u32);
  }
}

static void BM_u64(benchmark::State& state) {
  uint64_t u64 = 1;
  for (auto _ : state) {
    for (int i = 0; i < 100; ++i) {
      u64 = vmull_p64(u64, 0x01234567);
    }
    benchmark::DoNotOptimize(u64);
  }
}

BENCHMARK(BM_u32);
BENCHMARK(BM_u64);

int main(int argc, char* argv[]) {
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}
