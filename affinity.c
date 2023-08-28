#define _GNU_SOURCE

#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t tid = gettid();

    cpu_set_t *cpusetp = CPU_ALLOC(8);
    size_t size = CPU_ALLOC_SIZE(8);

    CPU_ZERO_S(size, cpusetp);
    CPU_SET_S(3, size, cpusetp);
    assert(sched_setaffinity(tid, size, cpusetp) == 0);
    printf("cpu=%d\n", sched_getcpu());

    CPU_ZERO_S(size, cpusetp);
    CPU_SET_S(6, size, cpusetp);
    assert(sched_setaffinity(tid, size, cpusetp) == 0);
    printf("cpu=%d\n", sched_getcpu());

    return 0;
}
