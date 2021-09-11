#include <atomic>
#include <iostream>
#include <thread>

// make sure value and ready are not in same cache line
struct {
  /// volatile char value;
  std::atomic<char> value;
  char padding[63];
  /// volatile bool ready;
  std::atomic<bool> ready;
} g __attribute ((aligned(64)));

int oo_count = 0;

#define wmb()   __asm__ __volatile__ ("dmb ish"   : : : "memory")
#define rmb()   __asm__ __volatile__ ("dmb ishld" : : : "memory")

void producer(void) {
///  g.value = 0x99;
///  wmb();
///  g.ready =  true;
  g.value.store(99, std::memory_order_relaxed);
  g.ready.store(true, std::memory_order_release);
}

void consumer(void) {
///  while (!g.ready)
///    ;
///  rmb();
///
///  if (g.value == 0) {
///    std::cout << "out of order observed!!!\n";
///    ++oo_count;
///  }

  while (!g.ready.load(std::memory_order_acquire))
    ;

  if (g.value.load(std::memory_order_relaxed) == 0) {
    std::cout << "out-of-order observed!!!\n";
    ++oo_count;
  }
}

int main(void) {
  long test_count = 0;

  while (1) {
    g.value = 0;
    g.ready = false;

    std::thread t1(consumer);
    std::thread t2(producer);

    t1.join();
    t2.join();

    ++test_count;
    if (test_count % 10000 == 0) {
      std::cout << test_count << ':' << oo_count << '\n';
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }

  return 0;
}
