#include "config.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include <atomic>
#include <thread>

static const int _send_size = _s2c_size;
static const int _recv_size = _c2s_size;
static std::atomic<int> _s_count{0};

__attribute__((noinline))
static long workload() {
  volatile long sum = 0;
  for (long i = 0; i < 12345; ++i) {
    sum += i;
  }
  return sum;
}

// return 0 on success, -1 on error ot timeout
__attribute__((noinline))
static int vio_wait(int s, bool is_read) {
  struct pollfd pfd{};
  pfd.fd = s;
  pfd.events = is_read ? POLLIN : POLLOUT;

  while (true) {
    const int ret = poll(&pfd, 1, -1);
    if (ret > 0) return 0;      // ready to send/recv
    if (ret == -1 && errno == EINTR) continue;
    return -1;  // error or timeout
  }
}

// return 0 on success, -999 if peer closed, -1 on error
__attribute__((noinline))
static ssize_t vio_read(int s, char* buf, int size) {
  while (true) {
    const ssize_t ret = recv(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;;
    if (ret == 0) return -999;    // peer closed
    if (ret > 0) { buf += ret, size -= ret; continue; }
    if (errno == EINTR) continue;
    if (errno != EAGAIN) return -1;
    if (vio_wait(s, /*is_read=*/true)) return -1;
  }
}

// return 0 on success, -1 on error
__attribute__((noinline))
static ssize_t vio_write(int s, const char* buf, int size) {
  while (true) {
    const ssize_t ret = send(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;
    if (ret >= 0) { buf += ret; size -= ret; continue; }
    if (errno == EINTR) continue;
    if (errno != EAGAIN) return -1;
    if (vio_wait(s, /*is_read=*/false)) return -1;
  }
}

long ns_diff(const struct timespec *t2, const struct timespec *t1)
{
    long s, ns;
    s = t2->tv_sec - t1->tv_sec;
    ns = t2->tv_nsec - t1->tv_nsec;
    return s * 1000000000LL + ns;
}

static void handler(int s) {
  fprintf(stderr, "server: new client(tid = %d), total=%d\n",
          int(gettid()), ++_s_count);

  char buf_recv[_recv_size];
  char buf_send[_send_size];
  memset(buf_recv, 0, _recv_size);
  memset(buf_send, 's', _send_size);

  int count = 0;
  long read_ns = 0, write_ns = 0;
  struct timespec t1, t2;

  while (true) {
    ++count;
    // get command
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    ssize_t ret = vio_read(s, buf_recv, _recv_size);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    read_ns += ns_diff(&t2, &t1);
    if (ret == -999) {
      fprintf(stderr, "server: client closed\n");
      break;
    }
    if (ret == -1) {
      fprintf(stderr, "server: read error\n");
      break;
    }
    for (ssize_t i = 0; i < _recv_size; ++i) {
      if (buf_recv[i] != 'c') {
        fprintf(stderr, "server: read mismatch\n");
        break;
      }
    }
    memset(buf_recv, 0, _recv_size);
    // do some work
    workload();
    // send response
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    ret = vio_write(s, buf_send, _send_size);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    if (ret) {
      fprintf(stderr, "server: write error\n");
      break;
    }
    write_ns += ns_diff(&t2, &t1);
    // print statistics
    if (count == 10000) {
      printf("read: %.2f us, write: %.2f us\n",
             double(read_ns)/count/1000.0, double(write_ns)/count/1000.0);
      count = 0;
      read_ns = write_ns = 0;
    }
  }

  close(s);
  fprintf(stderr, "server: drop client, total=%d\n", --_s_count);
}

int main() {
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) abort();

  const int reuse = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) abort();

  struct sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(_port);
  if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
    perror("server: bind");
    abort();
  }

  if (listen(s, 5) < 0) abort();

  while (true) {
    int sa_len = sizeof(sa);
    int s2 = accept(s, (struct sockaddr*)&sa, (socklen_t*)&sa_len);
    if (s2 == -1) {
      perror("server: accept");
      abort();
    }

    const int yes = 1;
    if (setsockopt(s2, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes))) abort();

    std::thread t(handler, s2);
    t.detach();
  }

  return 0;
}
