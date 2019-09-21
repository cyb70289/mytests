/* posix shared memory */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

static const int shm_sz = 65536;
static const char *shm_name = "/testshm1";

int main(int argc)
{
    int shm_fd, ret;
    char *addr;

    if (argc == 2) {
        /* fork and execed */
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        assert(shm_fd != -1);

        addr = mmap(NULL, shm_sz, PROT_READ | PROT_WRITE, MAP_SHARED,
                    shm_fd, 0);
        assert(addr != MAP_FAILED);

        for (int i = 0; i < shm_sz; ++i) {
            assert(addr[i] == 'A');
        }
        printf("A ok\n");

        memset(addr, 'B', shm_sz);
        return 0;
    }

    shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
    assert(shm_fd != -1);

    ret = ftruncate(shm_fd, shm_sz);
    assert(ret == 0);

    addr = mmap(NULL, shm_sz, PROT_READ | PROT_WRITE, MAP_SHARED,
                shm_fd, 0);
    assert(addr != MAP_FAILED);

    memset(addr, 'A', shm_sz);

    if (fork() == 0) {
        /* child */
        close(shm_fd);
        execl("./shm", "shm", "1", (char*)NULL);
        assert(0);
        return 1;
    }

    wait(NULL);

    for (int i = 0; i < shm_sz; ++i) {
        assert(addr[i] == 'B');
    }
    printf("B ok\n");

    close(shm_fd);
    shm_unlink(shm_name);

    return 0;
}
