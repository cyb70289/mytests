#include <chrono>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

static const int size = 1024*1024;
static char buf[size];

int main(int argc, char* argv[]) {
  if (argc != 2 || argv[1][0] != 'Z') {
    std::cout << "don't invoke directly, call via write.sh\n";
    return -1;
  }

  int fd = open("/tmp/__ramdisk__/ttt", O_CREAT|O_TRUNC|O_WRONLY, 0666);
  if (fd < 0) { perror("open"); return -1; }

  const auto t1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; ++i) {
    if (write(fd, buf, size) == -1) { perror("write"); return -1; }
  }
  fsync(fd);
  const auto t2 = std::chrono::high_resolution_clock::now();

  const std::chrono::duration<double> elapsed = t2 - t1;
  std::cout << elapsed.count()*1000 << " ms\n";

  return 0;
}
