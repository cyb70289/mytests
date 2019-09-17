/* SIGBUS when access passes end of file(up round to page boundary) */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

static const char *_f = "/tmp/__xxxxyyyy__";

static void handle(int signum)
{
    printf("%s\n", signum == SIGBUS ? "sigbus" : "sigsegv");
    _exit(1);
}

int main(void)
{
    int fd, ret;
    char *addr;
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handle;
    sigemptyset(&act.sa_mask);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);

    fd = open(_f, O_CREAT|O_RDWR|O_TRUNC, 0644);
    assert(fd != -1);
    ret = ftruncate(fd, 1111);
    assert(ret == 0);

    /* same result for shared and private mapping */
    addr = mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert(addr != (char *)-1);

    /* pass eof but in eof page boundary: not affect file */
    addr[2222] = 'a';

    if (fork() == 0) {
        /* mapped but pass eof page boundary: SIGBUS */
        addr[4444] = 'a';
        _exit(0);
    }

    if (fork() == 0) {
        /* not mapped page: SIGSEGV */
        addr[8888] = 'a';
        _exit(0);
    }

    wait(NULL);
    wait(NULL);

    return 0;
}
