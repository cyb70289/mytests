#include <thread>

volatile int x;
volatile int y;

const int n = 123456789;

#ifdef __aarch64__
#define mb()    __asm__ __volatile__ ("dmb ish":::"memory")
#else
#define mb()    __asm__ __volatile__ ("mfence":::"memory")
#endif

// program finishes in 0.16s w/o mb(), 4s w/ mb()

void thread1() {
    for (int i = 0; i < n; ++i) {
        x = 1;
        mb();
        y;
    }
}

void thread2() {
    for (int i = 0; i < n; ++i) {
        y = 1;
        mb();
        x;
    }
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();
    return 0;
}
