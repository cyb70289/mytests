#include <atomic>
#include <iostream>
#include <thread>

/// volatile int value __attribute ((aligned(64)));
/// volatile bool ready __attribute ((aligned(64)));
std::atomic<int> value __attribute ((aligned(64)));
std::atomic<bool> ready __attribute ((aligned(64)));

// how many times out of order issues are observed
std::atomic<int> oo_count{0};

#define wmb()   __asm__ __volatile__ ("dmb ish"   : : : "memory")
#define rmb()   __asm__ __volatile__ ("dmb ishld" : : : "memory")

void producer(void) {
///  value = 100;
///  wmb();
///  ready =  true;
  value.store(100, std::memory_order_relaxed);
  ready.store(true, std::memory_order_release);
}

void consumer(void) {
///  while (!ready)
///    ;
///  rmb();
///
///  if (value == 0) {
///    std::cout << "out of order observed!!!\n";
///    ++oo_count;
///  }

  while (!ready.load(std::memory_order_acquire))
    ;

  if (value.load(std::memory_order_relaxed) == 0) {
    std::cout << "out of order observed!!!\n";
    ++oo_count;
  }
}

int main(void) {
  long test_count = 0;

  while (1) {
    value = 0;
    ready = false;

    std::thread t1(consumer);
    std::thread t2(producer);

    t1.join();
    t2.join();

    ++test_count;
    if (test_count % 10000 == 0) {
      std::cout << test_count << ':' << oo_count << '\n';
    }
  }

  return 0;
}
