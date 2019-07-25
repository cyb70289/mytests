/*
 * All writes to fd opened with O_APPEND are always appended to file end,
 * no matter where file pointers is.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

static const char *_f = "/tmp/__xxxxyyyy__";

int main(void)
{
    int fd;
    off_t off;

    /* create a zero length file */
    fd = open(_f, O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* write "hello" */
    if (write(fd, "hello ", 6) != 6) {
        perror("write");
        return 1;
    }

    /* seek to head */
    off = lseek(fd, 0, SEEK_SET);
    printf("offset = %ld\n", (long)off);

    if (write(fd, "world\n", 6) != 6) {
        perror("write");
        return 1;
    }

    printf("check %s for results\n", _f);

    close(fd);
    return 0;
}
