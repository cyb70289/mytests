// gcc -fPIC -shared -o bypass-sve.so bypass-sve.c -ldl
// LD_PRELOAD=./bypass-sve.so ....

#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <sys/auxv.h>

#define ARM_COMPUTE_CPU_FEATURE_HWCAP_SVE       (1 << 22)
#define ARM_COMPUTE_CPU_FEATURE_HWCAP2_SVE2     (1 << 1)

unsigned long (*libc_getauxval)(unsigned long type) = NULL;

unsigned long getauxval(unsigned long type) {
  // XXX: NOT thread safe, just for testing purpose
  if (!libc_getauxval) {
    libc_getauxval = dlsym(RTLD_NEXT, "getauxval");
    if (!libc_getauxval) {
      fprintf(stderr, "dlsym failed: %s\n", dlerror());
      return 0;
    }
  }

  uint32_t hwcap = libc_getauxval(type);
  if (type == AT_HWCAP && (hwcap & ARM_COMPUTE_CPU_FEATURE_HWCAP_SVE)) {
    hwcap &= ~ARM_COMPUTE_CPU_FEATURE_HWCAP_SVE;
    fprintf(stderr, "getauxval: bypass SVE\n");
  } else if (type == AT_HWCAP2 && (hwcap & ARM_COMPUTE_CPU_FEATURE_HWCAP2_SVE2)) {
    hwcap &= ~ARM_COMPUTE_CPU_FEATURE_HWCAP2_SVE2;
    fprintf(stderr, "getauxval: bypass SVE2\n");
  }
  return hwcap;
}
