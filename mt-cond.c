#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int cnt = 0;    /* stop if cnt >= 3 */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *thread_wait3(void *arg)
{
    int n = 1;

    pthread_mutex_lock(&mtx);

    while (cnt < 3) {
        pthread_cond_wait(&cond, &mtx);
        printf("wait: %d\n", n++);
    }

    pthread_mutex_unlock(&mtx);

    return NULL;
}

void *thread_add(void *arg)
{
    for (int i = 0; i < 3; ++i) {
        sleep(1);
        pthread_mutex_lock(&mtx);
        ++cnt;
        pthread_mutex_unlock(&mtx);

        pthread_cond_signal(&cond);
    }

    return NULL;
}

int main(void)
{
    pthread_t t0, t1;

    pthread_create(&t0, NULL, thread_wait3, NULL);
    pthread_create(&t1, NULL, thread_add, NULL);

    pthread_join(t0, NULL);

    return 0;
}
