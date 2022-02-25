// https://quick-bench.com/q/MLvhY9_3IRZK0UR66DL1KFOF0xc

#include <nmmintrin.h>

// aligned to 128 bit boundary
alignas(16) char buf[65536 + 16];

const char* aligned_buf = buf + 16;
const char* unaligned_buf = buf + 16 - 8;

// fill indices and buffer with random data
void Prepare() {
  srand(42);
  for (int i = 0; i < sizeof(buf); ++i) {
    buf[i] = rand();
  }
}

// _mm_load_si128 for 16 bytes aligned read
static void Aligned(benchmark::State& state) {
  Prepare();
  for (auto _ : state) {
    __m128i r = _mm_set1_epi8(0);
    for (int i = 0; i < 65536/16; ++i) {
      __m128i v = _mm_load_si128((__m128i*)(aligned_buf + i * 16));
      r = _mm_or_si128(r, v);
    }
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(Aligned);

// _mm_lddqu_si128 for unaligned read
#pragma GCC target("sse4.2")
static void Unaligned(benchmark::State& state) {
  Prepare();
  for (auto _ : state) {
    __m128i r = _mm_set1_epi8(0);
    for (int i = 0; i < 65536/16; ++i) {
      __m128i v = _mm_lddqu_si128((__m128i*)(unaligned_buf + i * 16));
      r = _mm_or_si128(r, v);
    }
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(Unaligned);
