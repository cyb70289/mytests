// https://quick-bench.com/q/Iw42fZG_sZeomoRiUgzaWtcjXa4

#define N 4096

unsigned char a[N], b[N];
const unsigned char table[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3,
    4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4,
    4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4,
    5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5,
    4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
    3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
    5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4,
    5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

void init() {
  srand(42);
  for (int i = 0; i < N; ++i) {
    a[i] = rand();
    b[i] = rand();
  }
}

static void NoPopcount(benchmark::State& state) {
  init();
  for (auto _ : state) {
    unsigned char c[N];
    for (int i = 0; i < N; ++i) {
      c[i] = a[i] & b[i];
    }
    benchmark::DoNotOptimize(c[rand() % N]);
  }
}
BENCHMARK(NoPopcount);

static void WithPopcount(benchmark::State& state) {
  init();
  for (auto _ : state) {
    unsigned char c[N];
    unsigned int v = 0;
    for (int i = 0; i < N; ++i) {
      c[i] = a[i] & b[i];
      v += table[c[i]];
    }
    benchmark::DoNotOptimize(c[rand() % N]);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(WithPopcount);
