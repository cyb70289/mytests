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

const int _max_size = 512 * 1024 * 1024;

long ns_diff(const struct timespec *t2, const struct timespec *t1)
{
    long s, ns;
    s = t2->tv_sec - t1->tv_sec;
    ns = t2->tv_nsec - t1->tv_nsec;
    return s * 1000000000LL + ns;
}

int main(void)
{
    int fd, ret, sz;
    char *addr;
    char buf[1745];
    struct timespec t1, t2;

    memset(buf, 0x55, sizeof(buf));

    char* buf2 = malloc(_max_size);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    sz = 0;
    while (sz + sizeof(buf) <= _max_size) {
        memcpy(buf2 + sz, buf, sizeof(buf));
        sz += sizeof(buf);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("copy:  %ld ns\n", ns_diff(&t2, &t1));
    free(buf2);

    fd = open(_f1, O_CREAT|O_RDWR|O_TRUNC, 0644);
    assert(fd != -1);
    unlink(_f1);
    ret = ftruncate(fd, _max_size);
    assert(ret == 0);

    addr = mmap(NULL, _max_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert(addr != (char *)-1);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    sz = 0;
    while (sz + sizeof(buf) <= _max_size) {
        memcpy(addr + sz, buf, sizeof(buf));
        sz += sizeof(buf);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("mmap:  %ld ns\n", ns_diff(&t2, &t1));

    fd = open(_f2, O_CREAT|O_RDWR|O_TRUNC, 0644);
    assert(fd != -1);
    unlink(_f2);
    ret = ftruncate(fd, _max_size);
    assert(ret == 0);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    sz = 0;
    while (sz + sizeof(buf) <= _max_size) {
        ret = write(fd, buf, sizeof(buf));
        if (ret <= 0) abort();
        sz += ret;
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("write: %ld ns\n", ns_diff(&t2, &t1));

    return 0;
}
