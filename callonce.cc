/* g++ -std=c++11 -O3 -pthread -o callonce callonce.cc */

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>

struct CpuInfo {
    CpuInfo() { std::cout << "CpuInfo created\n"; }
    void Init() { std::cout << "CpuInfo initiated\n"; }
};

std::unique_ptr<CpuInfo> g_cpu_info;
static std::mutex cpuinfo_mutex;
static std::once_flag cpuinfo_once;

void GetInstance() {
#if 1   // 0 for mutex, 1 for call_once
    std::call_once(cpuinfo_once,
                   [](){ g_cpu_info.reset(new CpuInfo); g_cpu_info->Init(); });
#else
    std::lock_guard<std::mutex> lock(cpuinfo_mutex);
    if (!g_cpu_info) {
        g_cpu_info.reset(new CpuInfo);
        g_cpu_info->Init();
    }
#endif

  std::cout << "Got instance\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

int main(void) {
    std::thread st[16];

    for (int i = 0; i < 16; ++i) {
        st[i] = std::thread(GetInstance);
    }

    for (int i = 0; i < 16; ++i) {
        st[i].join();
    }

    return 0;
}
