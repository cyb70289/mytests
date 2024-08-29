// measure latency from core n to all other cores
// rewrite based on opensource ajakubek/core-latency

#include <cstdlib>
#include <cstring>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>

#include <numa.h>

const int _loops = 1000000;

static void pin_thread_to_cpu(int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  const pthread_t tid = pthread_self();
  if (pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset) != 0) {
    std::cerr << "pthread_setaffinity_np: " << std::strerror(errno) << '\n';
    std::exit(1);
  }
}

// test latency between src_cpu and peer_cpu
static void test_latency(int src_cpu, int peer_cpu, std::atomic<int>* shared,
                         bool busy_loop) {
  // stop_token
  // - 0: free to run
  // - 1: src cpu thread (this thread) tells helper thread to stop
  // - 2: peer cpu thread stopped and joinable
  std::atomic<int> stop_token{0};

  // initialize shared atomic to 0
  shared->store(0);

  // peer cpu thread loops setting shared variable to 1 if it's 0
  std::thread peer_thd(
      [peer_cpu, shared, &stop_token] {
        pin_thread_to_cpu(peer_cpu);
        int zero = 0;
        while (stop_token.load(std::memory_order_relaxed) == 0) {
          shared->store(1, std::memory_order_relaxed);
          while (!shared->compare_exchange_weak(zero, 1)) {
            zero = 0;
          }
        }
        stop_token.store(2);
      });

  // wait for helper thread entering the busy loop
  while (shared->load() == 0);

  // main thread loops setting shared variable to 0 if it's 1
  int one = 1;
  do {
    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < _loops; ++i) {
      shared->store(0, std::memory_order_relaxed);
      while (!shared->compare_exchange_weak(one, 0)) {
        one = 1;
      }
    }
    const auto end = std::chrono::high_resolution_clock::now();
    // output latency
    const std::chrono::duration<double, std::nano> duration = end - start;
    const double latency = duration.count() / _loops / 4;
    std::cout << "core " << src_cpu << " to " << peer_cpu
              << " latency = " << latency << "ns\n";
  } while (busy_loop);

  // wait for helper thread finishes
  stop_token.store(1);
  while (stop_token.load() != 2) {
    shared->store(0);
  }
  peer_thd.join();
}

// allocate a shared variable from heap or 1G hugetlb
// - heap:    random physical address, non-determined HN-F node
// - hugetlb: fixed physical address, possible to select HN-F node
static std::atomic<int>* alloc_atomic(
    const std::string& tlb1g_dir, int offset) {
  char* obj_addr;

  if (tlb1g_dir.empty()) {
    // allocate atomic variable from source cpu numa node
    obj_addr =
      reinterpret_cast<char*>( numa_alloc_local(sizeof(std::atomic<int>)));
  } else {
    const std::string tlb1g_file = tlb1g_dir + "/c2c-lat";
    int fd = open(tlb1g_file.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
      std::cerr << "open failed: " << tlb1g_file << '\n';
      std::exit(1);
    }
    void* addr = mmap(NULL, 1 << 30, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
      std::cerr << "mmap failed: " << std::strerror(errno) << '\n';
      std::exit(1);
    }
    // create atomic variable from hugepage offset
    std::cout << "shared variable at 1G hugepage offset " << offset << '\n';
    if (offset + sizeof(std::atomic<int>) > (1 << 30)) std::abort();
    obj_addr = reinterpret_cast<char*>(addr) + offset;
  }

  // use placement new to create object in pre-allocated buffer
  return new (obj_addr) std::atomic<int>(0);
}

static void usage(const char* program_name) {
  std::cout << "Usage: " << program_name << '\n'
            << " src-cpu [--peer-cpu=cpuid] [--offset=number] [--help|-h]\n"
            << "Options:\n"
            << "  src-cpu      source cpu id (default 0)\n"
            << "  --peer-cpu   test only src to peer cpu (optional)\n"
            << "  --hugetlbfs  mount point of 1G hugetlbfs (optional)/\n"
            << "  --offset     shared atomic address in hugepage (default 0)\n"
            << "  -h, --help   display this help message\n";
}

int main(int argc, char* argv[]) {
  const int n_cpus = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));

  int src_cpu = 0;
  int peer_cpu = -1;
  int offset = 0;
  std::string tlb1g_dir;

  const struct option long_opts[] = {
    {"help", no_argument, nullptr, 'h'},
    {"peer-cpu", required_argument, nullptr, 'p'},
    {"hugetlbfs", required_argument, nullptr, 't'},
    {"offset", required_argument, nullptr, 'o'},
    {nullptr, no_argument, nullptr, 0},
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "h", long_opts, nullptr)) != -1) {
    switch (opt) {
    case 'p':
      peer_cpu = std::atoi(optarg);
      if (peer_cpu < 0 || peer_cpu >= n_cpus) {
        std::cerr << "invalid peer-cpu id\n";
        return 1;
      }
      break;
    case 't':
      tlb1g_dir = optarg;
      break;
    case 'o':
      if (std::strlen(optarg) >= 2 && optarg[0] == '0') {
        if (optarg[1] == 'x' || optarg[1] == 'X') {
          offset = std::stoi(optarg, nullptr, 16);
        } else {
          offset = std::stoi(optarg, nullptr, 8);
        }
      } else if (std::strlen(optarg) >= 1 && std::isdigit(optarg[0])) {
        offset = std::atoi(optarg);
      } else {
        std::cerr << "invalid offset\n";
        return 1;
      }
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    default:
      usage(argv[0]);
      return 1;
    }
  };

  if (optind < argc) {
    src_cpu = std::atoi(argv[optind]);
  }
  if (src_cpu < 0 || src_cpu >= n_cpus) {
    std::cerr << "invalid src-cpu id\n";
    return 1;
  }
  if (src_cpu == peer_cpu) {
    std::cerr << "src-cpu must be different from peer-cpu\n";
    return 1;
  }

  std::cout << "test latency from core " << src_cpu;
  if (peer_cpu == -1) {
    std::cout << " to other cores\n";
  } else {
    std::cout << " to core " << peer_cpu << " repeatedly\n";
  }
  if (offset) {
    std::cout << "shared variable address: 0x"
              << std::hex << offset << std::dec << '\n';
  }

  // pin main thread to source cpu
  pin_thread_to_cpu(src_cpu);

  // allocate shared variable from heap (random) or hugetlb (fixed)
  std::atomic<int>* shared = alloc_atomic(tlb1g_dir, offset);

  if (peer_cpu == -1) {
    // test latency from source cpu to all other cpus
    for (int peer_cpu = 0; peer_cpu < n_cpus; ++peer_cpu) {
      if (peer_cpu != src_cpu) {
        test_latency(src_cpu, peer_cpu, shared, /*busy_loop=*/false);
      }
    }
  } else {
    // loop test latency from source cpu to peer cpu
    test_latency(src_cpu, peer_cpu, shared, /*busy_loop=*/true);
  }

  return 0;
}
