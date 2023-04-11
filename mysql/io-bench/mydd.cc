// to evaluate nvme queue usage under direct-io and async-io

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <linux/aio_abi.h>
#include <sys/syscall.h>

#include <thread>
#include <vector>

const long block_size = 8 * 1024;
const long block_count = 1024 * 1024;
alignas(4096) char iobuf[block_size];

/* Though each thread writes its own non-overlapped region, for direct-io,
 * nvme driver only uses one queue.
 *
 * fd: |--------------|--------------|--------------|------------
 *     ^  thd0 write  ^  thd1 write  ^  thd2 write  ^ ....
 */
static void evaluate_dio(int fd, int n_threads) {
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
}

static int io_setup(unsigned nr, aio_context_t *ctxp) {
    return syscall(__NR_io_setup, nr, ctxp);
}

static int io_destroy(aio_context_t ctx) {
    return syscall(__NR_io_destroy, ctx);
}

static int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp) {
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

static int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
                        struct io_event *events, struct timespec *timeout) {
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

static void evaluate_aio(int fd, int depth) {
    aio_context_t aio_ctx{};
    if (io_setup(depth, &aio_ctx)) {
        perror("io_setup");
        exit(1);
    }

    struct iocb cb{};
    struct iocb *cbs[1]{&cb};
    cb.aio_fildes = fd;
    cb.aio_lio_opcode = IOCB_CMD_PWRITE;
    cb.aio_buf = reinterpret_cast<uintptr_t>(iobuf);
    cb.aio_nbytes = block_size;

    int pending_io = 0;
    long sent_blocks = 0;
    while (sent_blocks < block_count) {
        if (pending_io < depth && (sent_blocks + pending_io) < block_count) {
            // queue one io
            cb.aio_offset = sent_blocks * block_size;
            if (io_submit(aio_ctx, 1, cbs) != 1) {
                perror("io_submit");
                exit(1);
            }
            ++pending_io;
        } else {
            // wait for one io done
            struct io_event events[1];
            if (io_getevents(aio_ctx, 1, 1, events, nullptr) != 1) {
                perror("io_getevents");
                exit(1);
            }
            --pending_io;
            ++sent_blocks;
        }
    }

    io_destroy(aio_ctx);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < block_size; ++i) {
        iobuf[i] = char(i);
    }

    bool aio = false;
    int depth = 1;

    if (argc >= 2) {
        depth = atoi(argv[1]);
        if (depth <= 0) depth = 1;
        if (argc == 3 && strcasecmp(argv[2], "aio") == 0) {
            aio = true;
            printf("AIO+DIO, queue depth = %d\n", depth);
        } else {
            printf("DIO, threads = %d\n", depth);
        }
    } else {
        printf("Usage:\n");
        printf("  ./mydd 32             direct-io with 32 threads\n");
        printf("  ./mydd 16 aio         aio + dio, queue depth = 16\n");
        return 1;
    }

    int fd = open("./__test__.bin", O_CREAT | O_TRUNC | O_WRONLY | O_DIRECT,
                  0644);
    if (fd < 0) { perror("open"); return 1; }

    if (aio) {
        evaluate_aio(fd, depth);
    } else {
        evaluate_dio(fd, depth);
    }

    close(fd);
    return 0;
}
