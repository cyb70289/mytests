/* https://preshing.com/20120515/memory-reordering-caught-in-the-act */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <atomic>

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

std::atomic<int> X, Y;
int r1, r2;

static void *thread1Func(void *param)
{
    for (;;) {
        sem_wait(&beginSema1);      // Wait for signal
        while (rand() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
#if 0
        X.store(1, std::memory_order_seq_cst);
        r1 = Y.load(std::memory_order_seq_cst);
#else
        int v = 0;
        X.compare_exchange_strong(v, 1, std::memory_order_acquire);
        r1 = Y.load(std::memory_order_relaxed);
#endif
        /* BUG on x86, good on Arm
         * X.store(1, std::memory_order_release);
         * r1 = Y.load(std::memory_order_acquire);
         */

        sem_post(&endSema);         // Notify transaction complete
    }

    return NULL;
};

static void *thread2Func(void *param)
{
    for (;;) {
        sem_wait(&beginSema2);      // Wait for signal
        while (rand() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
#if 0
        Y.store(1, std::memory_order_seq_cst);
        r2 = X.load(std::memory_order_seq_cst);
#else
        int v = 0;
        Y.compare_exchange_strong(v, 1, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_acquire);
        r2 = X.load(std::memory_order_relaxed);
#endif
        /* BUG on x86, good on Arm
         * Y.store(1, std::memory_order_release);
         * r2 = X.load(std::memory_order_acquire);
         */

        sem_post(&endSema);         // Notify transaction complete
    }

    return NULL;
};

int main(void)
{
    srand(time(NULL));

    // Initialize the semaphores
    sem_init(&beginSema1, 0, 0);
    sem_init(&beginSema2, 0, 0);
    sem_init(&endSema, 0, 0);

    // Spawn the threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread1Func, NULL);
    pthread_create(&thread2, NULL, thread2Func, NULL);

    // Repeat the experiment ad infinitum
    int detected = 0;
    for (int iterations = 1; ; iterations++) {
        // Reset X and Y
        X.store(0, std::memory_order_relaxed);
        Y.store(0, std::memory_order_relaxed);
        // Signal both threads
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        // Wait for both threads
        sem_wait(&endSema);
        sem_wait(&endSema);
        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0) {
            detected++;
            printf("%d reorders detected after %d iterations\n",
                    detected, iterations);
        }
    }

    return 0;
}
