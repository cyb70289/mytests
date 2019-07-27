#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

static const char *_f = "/tmp/__xxxxyyyy__";

#pragma GCC diagnostic ignored "-Wunused-result"

void print_file(int fd)
{
    int len;
    char buf[100];

    lseek(fd, 0, SEEK_SET);
    len = read(fd, buf, 100);
    buf[len] = 0;
    printf("%s\n", buf);
}

int main(void)
{
    int fd1, fd2, fd3, fdr;

    fd1 = open(_f, O_CREAT|O_WRONLY|O_TRUNC, 0644);
    fd2 = dup(fd1);
    fd3 = open(_f, O_WRONLY);

    fdr = open(_f, O_RDONLY);

    write(fd1, "hello,", 6);
    print_file(fdr);

    write(fd2, " world", 6);
    print_file(fdr);

    lseek(fd2, 0, SEEK_SET);
    write(fd1, "HELLO,", 6);
    print_file(fdr);

    write(fd3, "Gidday", 6);
    print_file(fdr);

    return 0;
}
