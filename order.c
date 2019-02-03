#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

const int consumers = 50;

sem_t sem_consumer_prepare, sem_done_prepare;
sem_t sem_consumer, sem_done;

volatile char start_producer = 0;

volatile char input[256];
volatile char input_changed[256];

volatile unsigned int caught = 0;

#define wmb()   __asm__ __volatile__ ("dmb ish"   : : : "memory")
#define rmb()   __asm__ __volatile__ ("dmb ishld" : : : "memory")

void *consumer(void *arg)
{
    while (1) {
        sem_wait(&sem_consumer_prepare);
        *input = 0x55;
        *input_changed = 0;
        sem_post(&sem_done_prepare);

        sem_wait(&sem_consumer);

        usleep(10);

        while (*input_changed == 0);
        if (*input == 0x55) {
            printf("caught!!!\n");
            ++caught;
        }

        sem_post(&sem_done);
    }

    return NULL;
}

int main(void)
{
    unsigned int cnt = 0;
    pthread_t tid[consumers];

    srand(time(NULL));

    sem_init(&sem_consumer_prepare, 0, 0);
    sem_init(&sem_done_prepare, 0, 0);
    sem_init(&sem_consumer, 0, 0);
    sem_init(&sem_done, 0, 0);

    for (int i = 0; i < consumers; ++i)
        pthread_create(&tid[i], NULL, consumer, NULL);

    while (1) {
        for (int i = 0; i < consumers; ++i)
            sem_post(&sem_consumer_prepare);

        for (int i = 0; i < consumers; ++i)
            sem_wait(&sem_done_prepare);

        for (int i = 0; i < consumers; ++i)
            sem_post(&sem_consumer);

        *input = 0xAA;
        wmb();
        *input_changed = 1;

        for (int i = 0; i < consumers; ++i)
            sem_wait(&sem_done);

        ++cnt;
        if (cnt % 1000 == 0)
            printf("%u:%u \n", cnt, caught);
    }

    return 0;
}
