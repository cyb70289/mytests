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

#include <thread>

static const int _send_size = _s2c_size;
static const int _recv_size = _c2s_size;

// return 0 on success, -1 on error or timeout
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
  fprintf(stderr, "server: new client %d\n", s);

  char buf_recv[_recv_size];
  char buf_send[_send_size];
  memset(buf_recv, 'c', _recv_size);
  memset(buf_send, 's', _send_size);

  // warm up
  for (int i = 0; i < 1000; ++i) {
    vio_read(s, buf_recv, _recv_size);
    usleep(1000);
    vio_write(s, buf_send, _send_size);
  }

  constexpr int kCount = 5000;
  int loops = 0;

  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);

  while (true) {
    struct timespec te;

    for (int i = 0; i < kCount; ++i) {
        // get command
        ssize_t ret = vio_read(s, buf_recv, _recv_size);
        if (ret == -999) {
          fprintf(stderr, "server: client closed\n");
          goto out;
        }
        if (ret == -1) {
          fprintf(stderr, "server: read error\n");
          goto out;
        }
        // short delay
        usleep(1000*1000 / kCount);
        // send response
        ret = vio_write(s, buf_send, _send_size);
        if (ret) {
          fprintf(stderr, "server: write error\n");
          goto out;
        }
    }

    // print statistics
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &te);
    const long time_ns = ns_diff(&te, &ts);
    ++loops;
    printf("read+write: %.2f us\n", double(time_ns)/kCount/loops/1000.0);
  }

out:
  fprintf(stderr, "server: drop client %d\n", s);
  close(s);
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

    handler(s2);
  }

  return 0;
}
