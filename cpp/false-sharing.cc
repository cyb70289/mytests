#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cstdio>
#include <cstdlib>

static long loops = 12345678;

static struct alignas(4096) {
  std::atomic<long> wr0;
  std::atomic<long> wr1;
#ifdef NO_FALSE_SHARING
  long pad[8];  // force wr, rd in different cacheline
#endif
  std::atomic<long> rd0;
  std::atomic<long> rd1;
} g_data[2] {{}};  // 0 initialized

static std::mutex g_mtx_thread_started, g_mtx_thread_run;
static std::condition_variable g_cv_thread_started, g_cv_thread_run;
static int g_thread_count = 0;
static bool g_thread_run = false;

static void reader(std::atomic<long>* rd0, std::atomic<long>* rd1) {
  {
    const std::lock_guard<std::mutex> lk(g_mtx_thread_started);
    ++g_thread_count;
  }
  g_cv_thread_started.notify_one();
  {
    std::unique_lock<std::mutex> lk(g_mtx_thread_run);
    g_cv_thread_run.wait(lk, [](){ return g_thread_run; });
  }
  while (true) {
    rd0->load();
    rd1->load();
  }
}

static void writer(std::atomic<long>* wr0, std::atomic<long>* wr1) {
  {
    const std::lock_guard<std::mutex> lk(g_mtx_thread_started);
    ++g_thread_count;
  }
  g_cv_thread_started.notify_one();
  {
    std::unique_lock<std::mutex> lk(g_mtx_thread_run);
    g_cv_thread_run.wait(lk, [](){ return g_thread_run; });
  }
  for (long i = 0; i < loops; ++i) {
    wr0->fetch_add(1);
    wr1->fetch_add(1);
  }
}

int main(int argc, char *argv[]) {
  if (argc > 1) loops = std::atoi(argv[1]);

  printf("cache line addr: %p, %p\n", &g_data[0], &g_data[1]);

  std::thread r0(reader, &g_data[0].rd0, &g_data[1].rd0);
  std::thread r1(reader, &g_data[0].rd1, &g_data[1].rd1);
  std::thread w0(writer, &g_data[0].wr0, &g_data[1].wr1);
  std::thread w1(writer, &g_data[0].wr0, &g_data[1].wr1);

  // wait for all threads ready
  {
    std::unique_lock<std::mutex> lk(g_mtx_thread_started);
    g_cv_thread_started.wait(lk, [](){ return g_thread_count == 4; });
  }
  // kick off threads run
  {
    const std::lock_guard<std::mutex> lk(g_mtx_thread_run);
    g_thread_run = true;
  }
  g_cv_thread_run.notify_all();

  // exit after any writer finishes
  w0.join();
  std::exit(0);

  return 0;
}
