/* trigger signal in signal handler and call itself recursively */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

static int in_handler;

static void handle_usr1(int signum)
{
    printf("enter %d\n", ++in_handler);

    if (in_handler <= 4) {
        sleep(1);
        kill(getpid(), SIGUSR1);
    }

    printf("leave %d\n", in_handler--);
}

int main(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_usr1;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER;
    sigaction(SIGUSR1, &act, NULL);

    kill(getpid(), SIGUSR1);

    return 0;
}
