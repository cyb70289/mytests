/*
 * open() will follow symlink and may cause security issue
 * To test:
 * - create a symlink to an arbitray file, even non-existent
 *   $ ln -s /tmp/__realfile__ /tmp/__symlink__
 * - run the program which manipulates the target file
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

    /* append O_EXCL fixes the issue */
    fd = open("/tmp/__symlink__", O_CREAT|O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (write(fd, "123456789", 9) != 9) {
        perror("write");
        return 1;
    }

    return 0;
}
