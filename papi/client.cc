#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "papi.h"
#ifdef WITH_URING
#include "uring.h"
#endif

// return 0 on success, -1 on error
#ifdef WITH_URING
static ssize_t vio_write(uring* uring, const char* buf, int size) {
  return uring->send(buf, size) ? 0 : -1;
}
#else
static ssize_t vio_write(int s, const char* buf, int size) {
  while (true) {
    ssize_t ret = send(s, buf, size, 0);
    if (ret == size) return 0;
    if (ret >= 0) { buf += ret; size -= ret; continue; }
    if (errno == EINTR) continue;
    if (ret) return -1;
  }
}
#endif

int main(int argc, char* argv[]) {
  const char* server_ip = "127.0.0.1";
  if (argc > 1) {
    server_ip = argv[1];
  }

  struct sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(55154);
  if (inet_pton(AF_INET, server_ip, &sa.sin_addr) <= 0) {
    fprintf(stderr, "clinet: malformed ip: %s\n", server_ip);
    return -1;
  }

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) abort();

  if (connect(s, (struct sockaddr*)&sa, sizeof(sa))) {
    perror("client: connect");
    abort();
  }

  const int y = 1;
  if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y))) abort();

#ifdef WITH_URING
  uring uring(s);
#endif

#ifdef __aarch64__
  papi papi("INST_RETIRED", "CPU_CYCLES");
#else
  papi papi(PAPI_TOT_INS, PAPI_TOT_CYC);
#endif

  char buf_send[64];
  memset(buf_send, 'c', 64);
  while (true) {
    papi.start();
    constexpr int loops = 1000;
    for (int i = 0; i < loops; ++i) {
#ifdef WITH_URING
      if (vio_write(&uring, buf_send, 64)) {
#else
      if (vio_write(s, buf_send, 64)) {
#endif
        fprintf(stderr, "client: error\n");
        abort();
      }
      usleep(100);
    }
    long insts, cycles;
    papi.stop(&insts, &cycles);
    printf("vio_write: insts = %ld, cycles= %ld, IPC = %.2f\n",
            insts / loops, cycles / loops, (double)insts / cycles);
  }

  return 0;
}
