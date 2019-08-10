/* syscall won't be interrupted by a signal if it's ignored */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

static void handle_alarm(int signum)
{
    /* printf is safe in this specific code */
    printf("ALARM\n");
}

int main(void)
{
    unsigned int ret;
    struct sigaction act, oldact;
    struct itimerval itv;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    memset(&itv, 0, sizeof(itv));
    itv.it_value.tv_usec = 900000;  /* 0.9s */

    /* handle signal */
    act.sa_handler = handle_alarm;
    sigaction(SIGALRM, &act, &oldact);

    setitimer(ITIMER_REAL, &itv, NULL);
    ret = sleep(3);
    printf("restart: sleep%s interrupted\n", ret == 0 ? " not" : "");

    /* restart syscall: sleep() cannot be restarted */
    sigaction(SIGALRM, &oldact, NULL);
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, NULL);

    setitimer(ITIMER_REAL, &itv, NULL);
    ret = sleep(3);
    printf("restart: sleep%s interrupted\n", ret == 0 ? " not" : "");

    /* ignore signal */
    signal(SIGALRM, SIG_IGN);

    setitimer(ITIMER_REAL, &itv, NULL);
    ret = sleep(3);
    printf("restart: sleep%s interrupted\n", ret == 0 ? " not" : "");
    
    return 0;
}
