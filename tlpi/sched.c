/*
 * - bind main thread to core-0, test threads to core-1
 * - normal threads share cpu evenly
 * - rt thread blocks normal thread`
 * - child thread inherits cpu affinity from its parent
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>

static pid_t gettid(void)
{
    return syscall(SYS_gettid);
}

static void sub_timespec(struct timespec *t2, struct timespec *t1)
{
    t2->tv_sec -= t1->tv_sec;
    t2->tv_nsec -= t1->tv_nsec;
    if (t2->tv_nsec < 0) {
        --t2->tv_sec;
        t2->tv_nsec += 1000000000LL;
    }
}

/* thread cleanup callback */
static void cleanup(void *arg)
{
    struct timespec t2;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
    sub_timespec(&t2, (struct timespec *)arg);

    printf("tid=%d, cputime=%u.%02us\n", gettid(),
            (unsigned)(t2.tv_sec), (unsigned)(t2.tv_nsec/10000000));
}

static void *normal(void *arg)
{
    struct timespec t1;
    cpu_set_t cset;

    CPU_ZERO(&cset);
    CPU_SET(1, &cset);

    /* test threads run on cpu-1 */
    if (sched_setaffinity(gettid(), sizeof(cset), &cset) == -1) {
        perror("sched_setaffinity");
        return NULL;
    }

    /* get thread processor time */
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
    pthread_cleanup_push(cleanup, &t1);

    while (1) {
        pthread_testcancel();
    }

    pthread_cleanup_pop(0);
    return NULL;
}

static void *realtm(void *arg)
{
    struct timespec t1, t2;
    const struct sched_param param = {
        .sched_priority = 1,
    };

    /* set rt scheduler */
    if (sched_setscheduler(gettid(), SCHED_RR, &param)) {
        perror("sched_setscheduler");
        exit(1);
    }

    return normal(arg);
}

static void test(void *(*thread1)(void *), void *(*thread2)(void *))
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    sleep(2);

    pthread_cancel(t1);
    pthread_cancel(t2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

int main(void)
{
    cpu_set_t cset;

    CPU_ZERO(&cset);
    CPU_SET(0, &cset);

    /* main thread runs on cpu-0 */
    if (sched_setaffinity(gettid(), sizeof(cset), &cset) == -1) {
        perror("sched_setaffinity");
        return 1;
    }

    printf("Run two normal threads for 2s\n");
    test(normal, normal);

    printf("Run one normal thread and one rt thread for 2s\n");
    test(realtm, normal);

    return 0;
}
