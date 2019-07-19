#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>

#define F "/tmp/__aaabbbcccddd__"

int main(void)
{
    int fd;
    char buf[4096];

    if ((fd = open(F, O_CREAT|O_TRUNC|O_RDWR)) < 0) {
        perror("open1");
        return 1;
    }

    if (write(fd, buf, 4096) != 4096) {
        perror("write");
        return 1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        perror("flock1");
        return 1;
    }

    if (mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0) == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    close(fd);

    if ((fd = open(F, 0)) < 0) {
        perror("fopen2");
        return 1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
        printf("flock works okay\n");
    else
        printf("flock NOT work!\n");

    //printf("press to exit.........\n");
    //getchar();

    return 0;
}
