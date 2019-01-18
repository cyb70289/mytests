/* print 010203040506 by 3 threads */

#include <stdio.h>
#include <pthread.h>

/* print 0,0,0,0,0,0 */
void *thread0(void *arg)
{
    pthread_mutex_t *mtx = arg;

    for (int i = 0; i < 6; ++i) {
        pthread_mutex_lock(&mtx[0]);
        fprintf(stderr, "0");
        pthread_mutex_unlock(&mtx[(i&1)+1]);
    }

    return NULL;
}

/* print 1,3,5 */
void *thread1(void *arg)
{
    pthread_mutex_t *mtx = arg;

    for (int i = 1; i <= 5; i += 2) {
        pthread_mutex_lock(&mtx[1]);
        fprintf(stderr, "%d", i);
        pthread_mutex_unlock(&mtx[0]);
    }

    return NULL;
}

/* print 2,4,6 */
void *thread2(void *arg)
{
    pthread_mutex_t *mtx = arg;

    for (int i = 2; i <= 6; i += 2) {
        pthread_mutex_lock(&mtx[2]);
        fprintf(stderr, "%d", i);
        pthread_mutex_unlock(&mtx[0]);
    }

    return NULL;
}

int main(void)
{
    pthread_t t0, t1, t2;
    pthread_mutex_t mtx[3] = {
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER,
    };

    pthread_mutex_lock(&mtx[1]);
    pthread_mutex_lock(&mtx[2]);

    pthread_create(&t0, NULL, thread0, mtx);
    pthread_create(&t1, NULL, thread1, mtx);
    pthread_create(&t2, NULL, thread2, mtx);

    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
