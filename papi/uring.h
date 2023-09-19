#include <liburing.h>

class uring {
 public:
  uring(int fd) : m_fd(fd) { io_uring_queue_init(1, &m_ring, 0); }
  ~uring() { io_uring_queue_exit(&m_ring); }

  bool send(const char* buf, int size) {
    io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    io_uring_prep_write(sqe, m_fd, buf, size, 0);
    return io_uring_submit_and_wait(&m_ring, 1) == 1;
  }

 private:
  io_uring m_ring;
  int m_fd;
};
