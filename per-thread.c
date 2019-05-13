#include <stdio.h>
#include <pthread.h>
#include <errno.h>

static __thread int per_thread;

void *thread(void *arg)
{
    printf("%p, %p\n", &per_thread, &errno);

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread, NULL);
    pthread_create(&t2, NULL, thread, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
