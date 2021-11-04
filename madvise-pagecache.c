// https://github.com/apache/arrow/pull/11588#issuecomment-958669125

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

const unsigned seed = 42;
const int N = 4;

// random read at most 1/N pages
int test_random_read(const char *p, size_t sz)
{
    srand(seed);

    int sum = 0;
    for (size_t i = 0; i < sz/4096/N; ++i) {
        double r = (double)rand() / RAND_MAX;
        r *= (sz - 2);
        sum += p[(size_t)r];
    }
    return sum;
}

int main(int argc, char *argv[])
{
    // test.bin is filled with 1G ramdon data
    int fd = open("./test.bin", O_RDONLY);
    if (fd < 0) abort();

    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) abort();
    const size_t sz = statbuf.st_size;

    char *p = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) abort();
    printf("%d pages mapped\n", (int)(sz/4096));

    // in my test box (linux 5.8, 16G ram)
    // - with madvise, 22% (1/N) file is in page cache when program finishes
    // - without  madvise, 98% file is in page cache
    if (argc == 1) {
        printf("without madvise\n");
    } else {
        if (strcmp(argv[1], "seq") == 0) {
            printf("with madvise(sequential)\n");
            if (posix_madvise(p, sz, POSIX_MADV_SEQUENTIAL) != 0) abort();
        } else if (strcmp(argv[1], "need") == 0) {
            printf("with madvise(willneed)\n");
            if (posix_madvise(p, sz, POSIX_MADV_WILLNEED) != 0) abort();
        } else if (strcmp(argv[1], "rand") == 0) {
            printf("with madvise(random)\n");
            if (posix_madvise(p, sz, POSIX_MADV_RANDOM) != 0) abort();
        } else if (strcmp(argv[1], "needrand") == 0) {
            printf("with madvise(willneed+random)\n");
            if (posix_madvise(p, sz, POSIX_MADV_WILLNEED) != 0) abort();
            if (posix_madvise(p, sz, POSIX_MADV_RANDOM) != 0) abort();
        }
        else {
            printf("unknown command [seq/need/rand/needrand]\n");
            return 1;
        }
    }

    int sum = test_random_read(p, sz);

    munmap(p, sz);
    return sum;
}
