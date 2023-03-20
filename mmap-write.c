// compare performance of mmap+msync vs. write+fsync

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

static const char *_f1 = "/tmp/__xxxxyyyy1__";
static const char *_f2 = "/tmp/__xxxxyyyy2__";

int _size = 1024 * 1024;    // size for each write
int _count = 512;           // total writes

long ns_diff(const struct timespec *t2, const struct timespec *t1)
{
    long s, ns;
    s = t2->tv_sec - t1->tv_sec;
    ns = t2->tv_nsec - t1->tv_nsec;
    return s * 1000000000LL + ns;
}

int main(int argc, char* argv[])
{
    if (argc == 3) {
        _size = atoi(argv[1]);
        _count = atoi(argv[2]);
        if (_size < 1024 || _count < 1) {
            printf("invalid argument\n");
            return 1;
        }
    }
    printf("size = %d, count = %d\n", _size, _count);

    int fd, ret;
    char *addr;
    char buf[1024];
    struct timespec t1, t2;

    memset(buf, 0x55, sizeof(buf));

    // mmap
    fd = open(_f1, O_CREAT|O_RDWR|O_TRUNC, 0644);
    assert(fd != -1);
    ret = ftruncate(fd, _size * _count);
    assert(ret == 0);

    addr = mmap(NULL, _size * _count,
                PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert(addr != (char *)-1);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    char *addr2 = addr;
    for (int i = 0; i < _count; ++i) {
        int sz = 0;
        while (sz + sizeof(buf) <= _size) {
            memcpy(addr2 + sz, buf, sizeof(buf));
            sz += sizeof(buf);
        }
        unsigned long offset = ((unsigned long)addr2) & 4095;
        ret = msync(addr2 - offset, sz + offset, MS_SYNC);
        if (ret) { perror("msync"); return 1; }
        ret = fdatasync(fd);
        if (ret) { perror("fdatasync"); return 1; }
        addr2 += sz;
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("mmap+msync:  %ld us\n", ns_diff(&t2, &t1) / 1000);
    munmap(addr, _size * _count);
    close(fd);
    unlink(_f1);

    // write
    fd = open(_f2, O_CREAT|O_RDWR|O_TRUNC, 0644);
    assert(fd != -1);
    ret = ftruncate(fd, _size * _count);
    assert(ret == 0);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    for (int i = 0; i < _count; ++i) {
        int sz = 0;
        while (sz + sizeof(buf) <= _size) {
            ret = write(fd, buf, sizeof(buf));
            if (ret <= 0) abort();
            sz += ret;
        }
        ret = fdatasync(fd);
        if (ret) abort();
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("write+fsync: %ld us\n", ns_diff(&t2, &t1) / 1000);
    close(fd);
    unlink(_f2);

    return 0;
}
