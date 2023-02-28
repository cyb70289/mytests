#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <atomic>
#include <thread>

static const int _max_threads = 256;

static const int _send_size = _c2s_size;
static const int _recv_size = _s2c_size;
static std::atomic<bool> _error_flag;
// per thread counter, no false sharing
static struct alignas(64) {
  std::atomic<long> v;
} _s_counts[_max_threads];

// return 0 on success, -1 on error
static ssize_t vio_read(int s, char* buf, int size) {
  while (true) {
    ssize_t ret = recv(s, buf, size, 0);
    if (ret == size) return 0;;
    if (ret == 0) return -1;    // peer closed
    if (ret > 0) { buf += ret, size -= ret; continue; }
    if (errno == EINTR) continue;
    return -1;
  }
}

// return 0 on success, -1 on error
static ssize_t vio_write(int s, const char* buf, int size) {
  while (true) {
    ssize_t ret = send(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;
    if (ret >= 0) { buf += ret; size -= ret; continue; }
    if (errno == EINTR) continue;
    if (ret) return -1;
  }
}

static void handler(int s, int idx) {
  char buf_recv[_recv_size];
  char buf_send[_send_size];
  memset(buf_recv, 0, _recv_size);
  memset(buf_send, 'c', _send_size);

  while (true) {
    // send command
    if (vio_write(s, buf_send, _send_size)) {
      fprintf(stderr, "client: write error\n");
      break;
    }
    // read response
    if (vio_read(s, buf_recv, _recv_size)) {
      fprintf(stderr, "client: read error\n");
      _error_flag.store(true, std::memory_order_relaxed);
      break;
    }
    _s_counts[idx].v.fetch_add(1, std::memory_order_relaxed);
  }

  close(s);
}

static long get_count(int n_threads) {
  long count = 0;
  for (int i = 0; i < n_threads; ++i) {
    count += _s_counts[i].v.load(std::memory_order_relaxed);
  }
  return count;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s threads server-ip\n", argv[0]);
    return -1;
  }

  const int n_threads = atoi(argv[1]);
  if (n_threads < 1 || n_threads > _max_threads) {
    fprintf(stderr, "threads must within [1, %d]\n", _max_threads);
    return -1;
  }
  fprintf(stderr, "client: threads = %d\n", n_threads);

  const char* server_ip = argv[2];
  struct sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(_port);
  if (inet_pton(AF_INET, server_ip, &sa.sin_addr) <= 0) {
    fprintf(stderr, "clinet: malformed ip: %s\n", server_ip);
    return -1;
  }

  for (int i = 0; i < n_threads; ++i) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) abort();

    if (connect(s, (struct sockaddr*)&sa, sizeof(sa))) {
      perror("client: connect");
      abort();
    }

    const int y = 1;
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y))) abort();

    std::thread t(handler, s, i);
    t.detach();
  }

  int now = 0;
  long prev_count = get_count(n_threads);
  while (true) {
    sleep(1);
    if (_error_flag.load(std::memory_order_relaxed)) return -1;
    ++now;
    const long curr_count = get_count(n_threads);
    printf("%d: tps = %ld\n", now, curr_count - prev_count);
    prev_count = curr_count;
  }

  return 0;
}
