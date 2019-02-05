/*
 * Experience weak memory order on Arm64
 *
 * INITIAL STATE:
 *     input = 0x55;
 *     input_changed = 0;
 *
 * PRODUCER:
 *     input = 0xAA;
 *     wmb();
 *     input_changed = 1;
 *
 * CONSUMER:
 *     while (input_changed == 0);
 *     // rmb() missing!
 *     if (input == 0x55) {
 *         out of order caught!
 *     }
 *
 * Number of reorderings caught:
 * - D05:         204 caught in 24 hours
 * - Centriq2400: 2267 caught in 24 hours
 * - ThunderX2:   easy to catch
 *
 * Build with gcc-7.3.0:
 *   gcc -O3 -Wall -march=native -pthread order.c -o order
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

const int consumers = 50;

/* happen to find this pattern to trigger memory order issues quickly */
struct {
    /*
     * only input[0], input_changed[0] is used, other stuffing bytes are just
     * to make sure they will NOT lie in same cache line.
     * */
    volatile char input[128];
    sem_t sem_consume;
    volatile char input_changed[128];
    sem_t sem_consume_done;
    sem_t sem_prepare;
    sem_t sem_prepare_done;
} D __attribute ((aligned(256)));

#define sem_prepare         D.sem_prepare
#define sem_prepare_done    D.sem_prepare_done
#define sem_consume         D.sem_consume
#define sem_consume_done    D.sem_consume_done
#define input               D.input
#define input_changed       D.input_changed

/* how many times out of order issues are observed */
volatile unsigned int caught = 0;

#define wmb()   __asm__ __volatile__ ("dmb ish"   : : : "memory")
#define rmb()   __asm__ __volatile__ ("dmb ishld" : : : "memory")

void *consumer(void *arg)
{
    while (1) {
        sem_wait(&sem_prepare);
        /* reset variables and introduce cache contentions */
        *input = 0x55;
        *input_changed = 0;
        sem_post(&sem_prepare_done);

        sem_wait(&sem_consume);

        usleep(10);

        while (*input_changed == 0);
#if 0
        rmb();  /* out of order observed if no rmb() */
#endif
        if (*input == 0x55) {
            printf("caught!!!\n");
            ++caught;
        }

        sem_post(&sem_consume_done);
    }

    return NULL;
}

int main(void)
{
    unsigned int cnt = 0;
    pthread_t tid[consumers];

    sem_init(&sem_prepare, 0, 0);
    sem_init(&sem_prepare_done, 0, 0);
    sem_init(&sem_consume, 0, 0);
    sem_init(&sem_consume_done, 0, 0);

    for (int i = 0; i < consumers; ++i)
        pthread_create(&tid[i], NULL, consumer, NULL);

    while (1) {
        /* release consumers to reset input, input_changed */
        for (int i = 0; i < consumers; ++i)
            sem_post(&sem_prepare);
        /* wait till all consumers prepared */
        for (int i = 0; i < consumers; ++i)
            sem_wait(&sem_prepare_done);

        /* release consumers for memory order testing */
        for (int i = 0; i < consumers; ++i)
            sem_post(&sem_consume);

        /* producer */
        *input = 0xAA;
        wmb();
        *input_changed = 1;

        /* wait till all consumers finished testing */
        for (int i = 0; i < consumers; ++i)
            sem_wait(&sem_consume_done);

        ++cnt;
        if (cnt % 1000 == 0)
            printf("%u:%u \n", cnt, caught);
    }

    return 0;
}
