/* child is adopted by init once parent exits or becomes zombie */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    if (fork()) {
        /* grandparent */
        sleep(2);
        /* reap parent */
        wait(NULL);
    } else if (fork()) {
        /* parent */
        printf("zombie = %u\n", getpid());
    } else {
        /* child */
        sleep(1);
        printf("parent is zombie, child ppid = %u\n", getppid());
        sleep(2);
        printf("parent is reaped, child ppid = %u\n", getppid());
    }

    return 0;
}
