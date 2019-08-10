/* printf is thread safe but not async-signal safe, deadlock happpens. */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

static int sigcount;

static void handle_alarm(int signum)
{
    ++sigcount;
    printf("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n");
}

int main(void)
{
    struct sigaction act;
    struct itimerval itv;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_alarm;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);

    memset(&itv, 0, sizeof(itv));
    itv.it_interval.tv_usec = itv.it_value.tv_usec = 100;

    if (setitimer(ITIMER_REAL, &itv, NULL) != 0) {
        perror("setitimer");
        return 1;
    }

    for (int i = 0; i < 100000; ++i)
        printf("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ\n");

    itv.it_interval.tv_usec = itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    printf("\nsignal handler called %d times\n", sigcount);

    return 0;
}
