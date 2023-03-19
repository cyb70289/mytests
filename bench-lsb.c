#include <stdio.h>
#include <string.h>
#include <time.h>

static inline size_t test_ffsll(size_t x)
{
    return ffsll(x) - 1;
}

// x != 0
static inline size_t test_optim(size_t x)
{
    size_t idx;
    asm("rbit %0,%1" : "=r"(idx) : "r"(x));
    return __builtin_clzll(idx);
}

long ns_diff(const struct timespec *t2, const struct timespec *t1)
{
    long s, ns;
    s = t2->tv_sec - t1->tv_sec;
    ns = t2->tv_nsec - t1->tv_nsec;
    return s * 1000000000LL + ns;
}

const int _loops = 123456;

int main(int argc, char* argv[])
{
    volatile unsigned char buf[4096];
    size_t s1 = 0, s2 = 0;
    struct timespec t1, t2;

    memset((void*)buf, 0x55, sizeof(buf));

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    for (int i = 0; i < _loops; ++i) {
        for (int i = 0; i < sizeof(buf); ++i) {
            s1 += test_optim(buf[i]);
        }
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("optim: %ld us\n", ns_diff(&t2, &t1) / 1000);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    for (int i = 0; i < _loops; ++i) {
        for (int i = 0; i < sizeof(buf); ++i) {
            s2 += test_ffsll(buf[i]);
        }
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    printf("ffsll: %ld us\n", ns_diff(&t2, &t1) / 1000);

    if (s1 != s2) printf("wrong!\n");

    return s1 - s2;
}
