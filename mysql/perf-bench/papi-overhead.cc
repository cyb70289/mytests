#include <iostream>

#include <papi.h>

static constexpr int _loop_count = 256 * 1024;

int main(int argc, char* argv[]) {
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

  long long values[5]{}, tmpv[5];
  for (int i = 0; i < _loop_count; ++i) {
    if (PAPI_start(EventSet) != PAPI_OK) abort();
    if (PAPI_stop(EventSet, tmpv) != PAPI_OK) abort();
    for (int i = 0; i < 5; ++i) {
      values[i] += tmpv[i];
    }
  }

  std::cout << "PAPI_start/stop overhead\n";
  std::cout << "inst:     " << values[0]/_loop_count << '\n';
  std::cout << "cycles:   " << values[1]/_loop_count << '\n';

  return 0;
}
