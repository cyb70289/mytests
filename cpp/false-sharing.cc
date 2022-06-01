#include <thread>

struct alignas(64) {
    volatile int a;
    //char pad[60];  // force a, b in different cacheline
    volatile int b;
} g;

const int n = 12345678;

int main() {
    // not atomic
    //std::thread t([]() { for (int i = 0; i < n; ++i) ++g.a; });
    //for (int i = 0; i < n; ++i) ++g.b;

    // atomic
    std::thread t([]() { for (int i = 0; i < n; ++i) __atomic_add_fetch(&g.a, 1, __ATOMIC_RELAXED); });
    for (int i = 0; i < n; ++i) __atomic_add_fetch(&g.b, 1, __ATOMIC_RELAXED);

    t.join();
    return 0;
}
