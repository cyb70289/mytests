/* use signal to sync parent/child */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void handle_usr1(int signum)
{
}

int main(void)
{
    struct sigaction act;
    sigset_t sigmask;
    pid_t pid;

    /* block SIGUSR1 */
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigmask, NULL);

    /* setup handler for SIGUSR1 */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_usr1;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, NULL);

    printf("child wait for parent\n");
    pid = fork();
    if (pid) {
        /* parent */
        sleep(1);
        kill(pid, SIGUSR1);
        printf("parent run\n");
        wait(NULL);
    } else {
        /* child */
        sigdelset(&sigmask, SIGUSR1);
        sigsuspend(&sigmask);
        printf("child run\n");
        _exit(0);
    }

    printf("\nparent wait for child\n");
    pid = fork();
    if (pid) {
        /* parent */
        sigdelset(&sigmask, SIGUSR1);
        sigsuspend(&sigmask);
        printf("parent run\n");
    } else {
        /* child */
        sleep(1);
        kill(getppid(), SIGUSR1);
        printf("child run\n");
        _exit(0);
    }

    return 0;
}
