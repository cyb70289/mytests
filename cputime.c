#include <stdio.h>
#include <time.h>
#include <unistd.h>

static inline long sub_timespec(struct timespec *t2, struct timespec *t1)
{
    t2->tv_sec -= t1->tv_sec;
    t2->tv_nsec -= t1->tv_nsec;
    t2->tv_nsec += 1000000000LL * t2->tv_sec;
}

void main() {
    struct timespec t1_wall, t2_wall, t1_cpu, t2_cpu;
    clock_gettime(CLOCK_MONOTONIC, &t1_wall);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1_cpu);
    sleep(1);
    clock_gettime(CLOCK_MONOTONIC, &t2_wall);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2_cpu);
    sub_timespec(&t2_wall, &t1_wall);
    sub_timespec(&t2_cpu, &t1_cpu);
    printf("wall=%d, cpu=%d\n", (int)t2_wall.tv_nsec, (int)t2_cpu.tv_nsec);
}
