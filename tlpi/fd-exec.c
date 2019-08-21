/* fork and execed file descriptor share same file structure with parent */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int fd, off;

    if (argc == 2) {
        /* fork and execed */
        fd = atoi(argv[1]);
        off = (int)lseek(fd, 0, SEEK_CUR);
        printf("child execed fd offset = %d, ", off);
        off = lseek(fd, 100, SEEK_CUR);
        printf("move to %d\n", off);
        return 0;
    }

    fd = open("/tmp/__xxxxyyyy__", O_CREAT|O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (fd != 4) {
        dup2(fd, 4);
        close(fd);
        fd = 4;
    }

    if (ftruncate(fd, 4096) == -1) {
        perror("ftruncate");
        return 1;
    }

    off = (int)lseek(fd, 80, SEEK_SET);
    printf("parent off = %d\n", off);

    if (fork() == 0) {
        /* Child */
        execl("./fd-exec", "fd-exec", "4", (char*)NULL);
    } else {
        /* Parent */
        wait(NULL);
        off = (int)lseek(fd, 0, SEEK_CUR);
        printf("parent off = %d\n", off);
    }

    close(fd);

    return 0;
}
