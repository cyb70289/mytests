#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "qspinlock.h"

static struct qspinlock qlock_;

extern struct qnode qnodes[MAX_USER_THREADS];
extern _Thread_local int this_thread_index;

// align counters to avoid false sharing
struct aligned_counter {
    volatile unsigned long count;
} __aligned(64);
static struct aligned_counter counters_[MAX_USER_THREADS];

static volatile sig_atomic_t stop_flag = 0;
static void sigint_handler(int signum) { stop_flag = 1; }

void* worker_thread(void *arg) {
    const int thread_index = *(int*)arg;
    free(arg);
    this_thread_index = thread_index;
    qnodes[thread_index].thread_index = thread_index;

    int old_cancel_type;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type);

    // bind thread to CPU
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(this_thread_index, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (1) {
        queued_spin_lock(&qlock_);

        // do some busy work
        for (volatile int i = 0; i < 1000; ++i);

        queued_spin_unlock(&qlock_);

        ++counters_[thread_index].count;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
#ifndef NDEBUG
    printf("!!!!! DEBUG build !!!!!\n");
#endif

    int thread_count = 18;

    // parse command-line arguments
    if (argc > 1) {
        thread_count = atoi(argv[1]);
        if (thread_count <= 0 || thread_count > MAX_USER_THREADS) {
            fprintf(stderr, "invalid thread count\n");
            return 1;
        }
    }
    printf("threads = %d\n", thread_count);

    // set up signal handler
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // create worker threads
    pthread_t threads[MAX_USER_THREADS];
    for (int i = 0; i < thread_count; ++i) {
        int *index = malloc(sizeof(int));
        *index = i;
        qnodes[i].thread_index = -1;
        pthread_create(&threads[i], NULL, worker_thread, index);
    }

    // print lock acquires for each thread once per second
    unsigned long prev_counts[MAX_USER_THREADS];
    memset(prev_counts, 0, sizeof(prev_counts));
    while (!stop_flag) {
        sleep(1);
        for (int i = 0; i < thread_count; ++i) {
            // XXX: there are data races of reading the counters here which are
            // updated in work threads, but it doesn't intefere much the result
            const unsigned long current_count = counters_[i].count;
            printf("%*lu", 8, current_count - prev_counts[i]);
            prev_counts[i] = current_count;
        }
        printf("\n");
    }

    // cancel all worker threads
    for (int i = 0; i < thread_count; ++i) {
        if (pthread_cancel(threads[i])) {
            perror("error cancelling thread\n");
            return 1;
        }
    }
    for (int i = 0; i < thread_count; ++i) {
        pthread_join(threads[i], NULL);
    }

    // dump lock and qnodes
    printf("=============== dump lock and qnodes status ===============\n");
    const int w = thread_count > 100 ? 3 : thread_count > 10 ? 2 : 1;
    const int tail_thread_index = (qlock_.tail >> _Q_TAIL_IDX_BITS) - 1;
    printf("[lock] locked=%d, pending=%d, tail_thread_index=%d\n",
            !!qlock_.locked, !!qlock_.pending, tail_thread_index);
    for (int i = 0; i < thread_count; ++i) {
        const struct mcs_spinlock *mcs = &qnodes[i].mcs;
        printf("[thread%-*d] mcs=%p: locked=%d, count=%d, next=%-*p",
                w, i, mcs, mcs->locked, mcs->count, 16, mcs->next);
        if (mcs->next) {
            // XXX: should use container_of to get qnode from mcs
            const struct qnode *qnode = (const struct qnode*)(mcs->next);
            printf(" [thread%d->thread%d]", i, qnode->thread_index);
        } else {
            printf(" [thread%d->null]", i);
        }
        printf("\n");
    }

    return 0;
}
