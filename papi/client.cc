#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <papi.h>

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

  long long insts = 0;
  int EventSet = PAPI_NULL;
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) abort();
  if (PAPI_set_domain(PAPI_DOM_ALL) != PAPI_OK) abort();
  if (PAPI_create_eventset(&EventSet) != PAPI_OK) abort();
  if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK) abort();

  char buf_send[64];
  memset(buf_send, 'c', 64);
  while (true) {
    if (PAPI_start(EventSet) != PAPI_OK) abort();
    for (int i = 0; i < 1000; ++i) {
      if (vio_write(s, buf_send, 64)) {
        fprintf(stderr, "client: error\n");
        abort();
      }
      usleep(100);
    }
    if (PAPI_stop(EventSet, &insts) != PAPI_OK) abort();
    printf("vio_write insts = %lld\n", insts / 1000);
    insts = 0;
  }

  return 0;
}
