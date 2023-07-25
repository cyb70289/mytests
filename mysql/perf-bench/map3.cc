#include <chrono>
#include <iostream>
#include <map>
#include <papi.h>

constexpr int _map_size = 128;
constexpr int _loop_count = 256*1024;

std::map<long, long> _map;

// key = 1, 2, ..., _map_size
void prepare_map() {
  for (long i = 0; i < _map_size; ++i) {
    _map[i+1] = i;
  }
}

__attribute__((noinline))
long bench(unsigned i) {
    auto it = _map.upper_bound(i&1);
    return it->second;
}

volatile int _v;
#define STR8    _v=1; _v=1; _v=1; _v=1; _v=1; _v=1; _v=1; _v=1
#define STR64   STR8; STR8; STR8; STR8; STR8; STR8; STR8; STR8
#define STR512  STR64; STR64; STR64; STR64; STR64; STR64; STR64; STR64
#define STR4K   STR512; STR512; STR512; STR512; STR512; STR512; STR512; STR512
#define STR32K  STR4K; STR4K; STR4K; STR4K; STR4K; STR4K; STR4K; STR4K

#pragma GCC optimize ("align-functions=65536")
__attribute__((noinline))
void pollute_icache() {
  STR32K;
}

int main(int argc, char* argv[]) {
  prepare_map();

  int EventSet = PAPI_NULL;
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) abort();
  if (PAPI_create_eventset(&EventSet) != PAPI_OK) abort();
#ifdef __aarch64__
  if (PAPI_add_named_event(EventSet, "INST_RETIRED") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "CYCLES") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "L1I_CACHE_REFILL") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "L1D_CACHE_REFILL") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "BR_RETIRED") != PAPI_OK) abort();
#else // x86_64
  if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK) abort();
  if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "ICACHE_64B.IFTAG_MISS") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "L1D:REPLACEMENT") != PAPI_OK) abort();
  if (PAPI_add_named_event(EventSet, "BR_INST_RETIRED.ALL_BRANCHES") != PAPI_OK) abort();
#endif

  long long values[5]{};
  long long tmpv[5]{};

  long s = 0;
  for (unsigned  i = 0; i < _loop_count; ++i) {
    pollute_icache();
    if (PAPI_start(EventSet) != PAPI_OK) abort();
    s += bench(i);
    if (PAPI_stop(EventSet, tmpv) != PAPI_OK) abort();
    for (int i = 0; i < 5; ++i) {
      values[i] += tmpv[i];
    }
  }

  std::cout << "inst:     " << values[0]/_loop_count << '\n';
  std::cout << "cycles:   " << values[1]/_loop_count << '\n';
  std::cout << "l1i-miss: " << values[2]/_loop_count << '\n';
  std::cout << "l1d-miss: " << values[3]/_loop_count << '\n';
  std::cout << "br:       " << values[4]/_loop_count << '\n';
  std::cout << "IPC:      " << (double)values[0]/values[1] << '\n';
  return s;
}
