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
#include <sys/epoll.h>

#include <papi.h>

// return 0 on success, -1 on error ot timeout
#ifdef WITH_EPOLL
static int vio_wait(int epollfd) {
  while (true) {
    struct epoll_event event;
    const int ret = epoll_wait(epollfd, &event, 1, -1);
    if (ret > 0) return 0;      // ready
    if (ret == -1 && errno == EINTR) continue;
    return -1;  // error or timeout
  }
}
#else
static int vio_wait(int s) {
  struct pollfd pfd{};
  pfd.fd = s;
  pfd.events = POLLIN;

  while (true) {
    const int ret = poll(&pfd, 1, -1);
    if (ret > 0) return 0;      // ready
    if (ret == -1 && errno == EINTR) continue;
    return -1;  // error or timeout
  }
}
#endif

// return 0 on success, -999 if peer closed, -1 on error
static ssize_t vio_read(int s, int pollfd, char* buf, int size) {
  while (true) {
    const ssize_t ret = recv(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;;
    if (ret == 0) return -999;    // peer closed
    if (ret > 0) { buf += ret, size -= ret; continue; }
    if (errno == EINTR) continue;
    if (errno != EAGAIN) return -1;
    if (vio_wait(pollfd)) return -1;
  }
}

int main() {
#ifdef WITH_EPOLL
  printf("!!!! epoll !!!!\n");
#else
  printf("!!!! poll !!!!\n");
#endif

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) abort();

  const int reuse = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) abort();

  struct sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(55154);
  if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
    perror("server: bind");
    abort();
  }

  if (listen(s, 5) < 0) abort();

  long long insts = 0;
  int EventSet = PAPI_NULL;
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) abort();
  if (PAPI_set_domain(PAPI_DOM_ALL) != PAPI_OK) abort();
  if (PAPI_create_eventset(&EventSet) != PAPI_OK) abort();
  if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK) abort();

  while (true) {
    int sa_len = sizeof(sa);
    int s2 = accept(s, (struct sockaddr*)&sa, (socklen_t*)&sa_len);
    if (s2 == -1) {
      perror("server: accept");
      abort();
    }

    const int yes = 1;
    if (setsockopt(s2, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes))) abort();

    int pollfd = s2;
#ifdef WITH_EPOLL
    int epollfd = epoll_create1(0);
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = s2;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, s2, &ev) == -1) abort();
    pollfd = epollfd;
#endif

    char buf_recv[64];
    bool ok = true;
    while (ok) {
      memset(buf_recv, 0, 64);
      if (PAPI_start(EventSet) != PAPI_OK) abort();
      for (int i = 0; i < 1000; ++i) {
        ssize_t ret = vio_read(s2, pollfd, buf_recv, 64);
        if (ret) {
          fprintf(stderr, "server: error\n");
          ok = false;
          break;
        }
      }
      if (PAPI_stop(EventSet, &insts) != PAPI_OK) abort();
      if (ok) {
        printf("vio_read insts = %lld\n", insts / 1000);
        insts = 0;
        for (int i = 0; i < 64; ++i) {
          if (buf_recv[i] != 'c') abort();
        }
      }
    }

    close(s2);
#ifdef WITH_EPOLL
    close(epollfd);
#endif
  }

  return 0;
}
