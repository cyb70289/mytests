/*
 * Test case to show performance improvement from c++11 memory model
 *
 * Benchmark steps:
 * 1. copy memory block1
 * 2. lock()
 * 3. copy memory block2
 * 4. unlock()
 * 5. repeat step 1-4
 *
 * In original mysql code, lock/unlock is implemented with gcc __sync_xxx
 * atomic intrinsics, which forces full memory barrier. It will cause stall
 * between step1 and step2. This is not necessary for normal lock use case.
 * Optimized code leverages c++11 load-acquire and store-release memory
 * model, it protects step3 but gives cpu freedom to optimize memory access
 * of step1.
 *
 * We can see significant performance boost on ThunderX2. See below test
 * result for reference (smaller time means faster):
 *
 * >> $ make
 *
 * >> $ ./bench-lock
 *
 * >> bench orignal code (full memory barrier)
 * >> time: 1588 ms
 *
 * >> bench optimized code (half memory barrier)
 * >> time: 1407 ms
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "os0atomic.h"

static const int _loops = 40000000;
static const int _bufsz = 16*1024;

static char _buf1[_bufsz], _buf2[_bufsz];
static char _buf3[_bufsz], _buf4[_bufsz];

static bool lock_original(volatile int *lock)
{
    /* __sync_bool_compare_and_swap */
    return os_compare_and_swap(lock, 0, 1);
}

static void unlock_original(volatile int *lock)
{
    /* __sync_bool_compare_and_swap */
    os_compare_and_swap(lock, 1, 0);
}

static bool lock_optimized(os_atomic_t<int> *lock)
{
    int expected = 0;
    return lock->compare_exchange_strong(expected, 1, order_acquire);
}

static void unlock_optimized(os_atomic_t<int> *lock)
{
    int expected = 1;
    lock->compare_exchange_strong(expected, 0, order_release);
}

static void test_lock_original(void)
{
    volatile int lock = 0;

    for (int i = 0; i < _loops; ++i) {
        memcpy(_buf1, _buf2, _bufsz);
        lock_original(&lock);
        memcpy(_buf3, _buf4, _bufsz);
        unlock_original(&lock);
    }
}

static void test_lock_optimized(void)
{
    os_atomic_t<int> lock;
    lock.store(0, order_relaxed);

    for (int i = 0; i < _loops; ++i) {
        memcpy(_buf1, _buf2, _bufsz);
        lock_optimized(&lock);
        memcpy(_buf3, _buf4, _bufsz);
        unlock_optimized(&lock);
    }
}

static void show_timediff(struct timeval *tv1, struct timeval *tv2)
{
    double time = tv2->tv_usec - tv1->tv_usec;
    time = time / 1000 + (tv2->tv_sec - tv1->tv_sec) * 1000;
    printf("time: %.0f ms\n\n", time);
}

int main(void)
{
    struct timeval tv1, tv2;

    /* warm up */
    test_lock_original();

    printf("bench orignal code (full memory barrier)\n");
    gettimeofday(&tv1, 0);

    test_lock_original();

    gettimeofday(&tv2, 0);
    show_timediff(&tv1, &tv2);

    printf("bench optimized code (half memory barrier)\n");
    gettimeofday(&tv1, 0);

    test_lock_optimized();

    gettimeofday(&tv2, 0);
    show_timediff(&tv1, &tv2);

    return 0;
}
