// to experience nvme queue usage
// - buffer io: queue used effectively by nvme driver
// - direct io: queue not effectively used

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <thread>
#include <vector>

const long block_size = 8 * 1024;
const long block_count = 1024 * 1024;
alignas(4096) char iobuf[block_size]{};

// Evaluate if nvme queues are used effectively
// Usage:
// - ./mydd direct 32    --> direct-io with 32 threads
// - ./mydd              --> buffered-io (1 thread)
int main(int argc, char *argv[]) {
    int o_direct = 0;
    int n_threads = 1;
#ifdef O_DIRECT
    if (argc > 1 && strcasecmp(argv[1], "direct") == 0) {
        o_direct = O_DIRECT;
        printf("enable direct io\n");
        if (argc == 3) {
            n_threads = atoi(argv[2]);
            if (n_threads <= 0) n_threads = 1;
        }
        printf("threads = %d\n", n_threads);
    }
#endif

    int fd = open("./__test__.bin", O_CREAT | O_TRUNC | O_WRONLY | o_direct,
                  0644);
    if (fd < 0) { perror("open"); return 1; }

    /* Though each thread writes its own non-overlapped region, for direct-io,
     * looks nvme driver only uses one queue.
     *
     * fd: |--------------|--------------|--------------|------------
     *     ^  thd0 write  ^  thd1 write  ^  thd2 write  ^ ....
     */

    std::vector<std::thread> threads;
    for (int i = 0; i < n_threads; ++i) {
        threads.emplace_back(
            [fd, n_threads, idx = i] {
                off_t offset = (block_count * block_size) / n_threads * idx;
                for (int i = 0; i < block_count / n_threads; ++i) {
                    if (pwrite(fd, iobuf, block_size, offset) != block_size) {
                        perror("pwrite");
                        exit(1);
                    }
                    offset += block_size;
                }
            }
        );
    }
    for (int i = 0; i < n_threads; ++i) {
        threads[i].join();
    }

    close(fd);
    return 0;
}
