// gcc -O3 -march=armv8-a+lse cas-lat.c

#include <stdio.h>

int main() {
  int i = 1;
  int one = 1, two = 2;

  long start_cycle;
  asm volatile("mrs %0, cntvct_el0" : "=r"(start_cycle));

  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &one, 2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange_n(&i, &two, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

  long end_cycle;
  asm volatile("mrs %0, cntvct_el0" : "=r"(end_cycle));
  printf("cycles = %ld\n", end_cycle - start_cycle);
  if (one != 1 || two != 2) {
    printf("RESULT may be not valid!\n");
  }

  return 0;
}
