#include <random>
#include <vector>
#include <cmath>

const int N = 100000;
using T = double;  // big diff for double, no diff for int

std::vector<T> prepare() {
  std::mt19937 mt(42);
  std::uniform_real_distribution<double> dist(1.0, 10000.0);

  std::vector<T> v(N);
  for (int i = 0; i < N; ++i) {
    v[i] = static_cast<T>(dist(mt));
  }
  return v;
}

static void bench_power(benchmark::State& state) {
  const auto v = prepare();
  for (auto _ : state) {
    double s = 0;
    for (int i = 0; i < N; ++i) {
      s += std::pow(v[i], 0.5);
    }
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(bench_power);

static void bench_sqrt(benchmark::State& state) {
  const auto v =  prepare();
  for (auto _ : state) {
    double s = 0;
    for (int i = 0; i < N; ++i) {
      s += std::sqrt(v[i]);
    }
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(bench_sqrt);
