#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int daemonize(void)
{
    pid_t pid;
    int fd, maxfd;

    /* drop process group leader */
    pid = fork();
    if (pid == (pid_t)-1) {
        return -1;
    } else if (pid) {
        _exit(0);
    }

    /* child: drop session leader */
    if (setsid() == (pid_t)-1) {
        return -1;
    }
    pid = fork();
    if (pid == (pid_t)-1) {
        return -1;
    } else if (pid) {
        _exit(0);
    }

    /* grandchild: non process group leader, non session leader */

    umask(0);
    chdir("/");

    /* close all fd */
    maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1)
        maxfd = 8192;
    for (fd = 0; fd < maxfd; ++fd) {
        close(fd);
    }

    /* redirect stdio to /dev/null */
    fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) {
        return -1;
    }
    if (dup2(STDIN_FILENO, STDOUT_FILENO) == -1) {
        return -1;
    }
    if (dup2(STDIN_FILENO, STDERR_FILENO) == -1) {
        return -1;
    }

    return 0;
}

int main(void)
{
    if (daemonize() == -1) {
        return 1;
    }

    sleep(10);

    return 0;
}
