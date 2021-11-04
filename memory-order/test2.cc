#include <future>
#include <iostream>
#include <thread>

#ifdef __aarch64__
#define mb()    __asm__ __volatile__("dmb ish" : : : "memory")
#else
#define mb()    __asm__ __volatile__("mfence")
#endif
 
volatile int x, y;

int f1() {
  x = 1;
  return y;
}

int f2() {
  y = 1;
  return x;
}

int main(void) {
  long test_count = 0, oo_count = 0;

  while (1) {
    x = y = 0;

    auto fut1 = std::async(f1);
    auto fut2 = std::async(f2);
    int r1 = fut1.get();
    int r2 = fut2.get();

    if (r1 == 0 && r2 == 0) {
      std::cout << "out of order observed!!!\n";
      ++oo_count;
    }

    ++test_count;
    if (test_count % 10000 == 0) {
      std::cout << test_count << ':' << oo_count << '\n';
    }
  }

  return 0;
}
