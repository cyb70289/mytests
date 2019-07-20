/*
 * forked or duped file descriptors share same file structure.
 * Manipulating one fd affects all other fds.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    int fd, off, wstatus;
    
    fd = open("/tmp/__xxxxyyyy__", O_CREAT|O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    off = (int)lseek(fd, 0, SEEK_CUR);
    printf("parent off = %d\n", off);

    if (fork() == 0) {
        /* Child */
        printf("child set off to 100\n");
        if (lseek(fd, 100, SEEK_SET) == (off_t)-1) {
            perror("child lseek");
            return 1;
        }
    } else {
        /* Parent */
        wait(&wstatus);
        off = (int)lseek(fd, 0, SEEK_CUR);
        printf("parent off = %d\n", off);
    }

    close(fd);

    return 0;
}
