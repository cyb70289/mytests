#include <atomic>
#include <thread>

struct alignas(64) {
    std::atomic<int> a;
#if 0
    char pad[60];
#endif
    std::atomic<int> b;
} g;

const int n = 12345678;

int main() {
    std::thread t([]() { for (int i = 0; i < n; ++i) ++g.a; });
    for (int i = 0; i < n; ++i) ++g.b;
    t.join();
    return 0;
}
