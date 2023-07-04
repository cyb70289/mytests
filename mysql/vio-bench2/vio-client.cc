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

#include <thread>

static const int _send_size = _c2s_size;
static const int _recv_size = _s2c_size;
static bool _error_flag = false;

// return 0 on success, -1 on error
static ssize_t vio_read(int s, char* buf, int size) {
  while (true) {
    ssize_t ret = recv(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;;
    if (ret == 0) return -1;    // peer closed
    if (ret > 0) { buf += ret, size -= ret; continue; }
    if (errno == EINTR || errno == EAGAIN) continue;
    if (ret) return -1;
  }
}

// return 0 on success, -1 on error
static ssize_t vio_write(int s, const char* buf, int size) {
  while (true) {
    ssize_t ret = send(s, buf, size, MSG_DONTWAIT);
    if (ret == size) return 0;
    if (ret >= 0) { buf += ret; size -= ret; continue; }
    if (errno == EINTR || errno == EAGAIN) continue;
    if (ret) return -1;
  }
}

static void handler(int s) {
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
      _error_flag = true;
      break;
    }
  }

  close(s);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s server-ip\n", argv[0]);
    return -1;
  }

  const char* server_ip = argv[1];
  struct sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(_port);
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

  handler(s);

  return 0;
}
