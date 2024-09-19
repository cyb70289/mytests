#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifdef BENCH_QSPINLOCK
#include "qspinlock.h"
static struct qspinlock qlock_;
extern struct qnode qnodes[MAX_USER_THREADS];
extern _Thread_local int this_thread_index;
#else
#define MAX_USER_THREADS	64
#define __aligned(x)	    __attribute__((__aligned__(x)))
static pthread_spinlock_t slock_;
#endif

// align counters to avoid false sharing
struct aligned_counter {
    volatile unsigned long count;
} __aligned(64);
static struct aligned_counter counters_[MAX_USER_THREADS];

void* worker_thread(void *arg) {
    const int thread_index = *(int*)arg;
    free(arg);
#ifdef BENCH_QSPINLOCK
    this_thread_index = thread_index;
    qnodes[thread_index].thread_index = thread_index;
#endif

    // bind thread to CPU
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_index, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (1) {
        // hold lock: 100 ~ 200
        int rand_loops = rand() % 100 + 100;

#ifdef BENCH_QSPINLOCK
        queued_spin_lock(&qlock_);
#else
        pthread_spin_lock(&slock_);
#endif

        // do some busy work
        for (volatile int i = 0; i < rand_loops; ++i);

#ifdef BENCH_QSPINLOCK
        queued_spin_unlock(&qlock_);
#else
        pthread_spin_unlock(&slock_);
#endif

        ++counters_[thread_index].count;

        // free lock: 0 ~ 100
        rand_loops = rand() % 100;  // 0 ~ 100
        for (volatile int i = 0; i < rand_loops; ++i);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    srand(42);

    // get number of threads from command-line (default 16)
    // e.g., ./test-lse 18 --> 18 threads
    int thread_count = 16;
    if (argc > 1) {
        thread_count = atoi(argv[1]);
        if (thread_count <= 0 || thread_count > MAX_USER_THREADS) {
            fprintf(stderr, "invalid thread count\n");
            return 1;
        }
    }
    printf("threads = %d\n", thread_count);

    // create worker threads
    pthread_t threads[MAX_USER_THREADS];
    for (int i = 0; i < thread_count; ++i) {
        int *index = malloc(sizeof(int));
        *index = i;
#ifdef BENCH_QSPINLOCK
        qnodes[i].thread_index = -1;
#endif
        pthread_create(&threads[i], NULL, worker_thread, index);
    }

    // print lock acquires for each thread once per second
    unsigned long prev_counts[MAX_USER_THREADS];
    memset(prev_counts, 0, sizeof(prev_counts));
    while (1) {
        sleep(1);
        unsigned long sum = 0;
        for (int i = 0; i < thread_count; ++i) {
            // XXX: there are data races of reading the counters here which are
            // updated in work threads, but it doesn't intefere much the result
            const unsigned long current_count = counters_[i].count;
            printf("%*lu", 8, current_count - prev_counts[i]);
            sum += current_count - prev_counts[i];
            prev_counts[i] = current_count;
        }
        printf("   TOTAL: %lu\n", sum);
    }

    return 0;
}
