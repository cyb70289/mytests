/*
 * Four processes open and write to one file concurrently.
 * Though writes are interleaved, a single write is always clean.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define _n      4
#define _sem1   "/mysem_ready"
#define _sem2   "/mysem_go"

static char _s[_n][60];

static const char *_f = "/tmp/__xxxxyyyy__";

static int child(int fd, int idx)
{
    sem_t *sem1, *sem2;

    sem1 = sem_open(_sem1, 0);
    sem2 = sem_open(_sem2, 0);
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        perror("child sem_open");
        return 1;
    }

    sem_post(sem1);
    sem_wait(sem2);

    for (int i = 0; i < 100; ++i) {
        if (write(fd, _s[idx], sizeof(_s[idx])) != sizeof(_s[idx])) {
            perror("child write");
            return 1;
        }
    }

    close(fd);
    return 0;
}

int main(void)
{
    int fd, st;
    sem_t *sem1, *sem2;

    /* create a zero length file */
    fd = open(_f, O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* prepare test buffer */
    for (int i = 0; i < _n; ++i) {
        memset(_s[i], 'a'+i, sizeof(_s[i])-1);
        _s[i][sizeof(_s[i])-1] = '\n';
    }

    /* create semaphores */
    sem1 = sem_open(_sem1, O_CREAT, 0666, 0);
    sem2 = sem_open(_sem2, O_CREAT, 0666, 0);
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    /* fork n processes to write concurrently */
    for (int i = 0; i < _n; ++i) {
        if (fork() == 0)
            return child(fd, i);
    }
    close(fd);

    /* wait for all processes ready */
    for (int i = 0; i < _n; ++i)
        sem_wait(sem1);

    /* let all processes go */
    for (int i = 0; i < _n; ++i)
        sem_post(sem2);

    /* wait all childs done */
    for (int i = 0; i < _n; ++i)
        wait(&st);

    printf("check %s for results\n", _f);

    return 0;
}
