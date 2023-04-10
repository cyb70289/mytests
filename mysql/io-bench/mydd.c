// to experience nvme queue usage
// - buffer io: queue used effectively by nvme driver
// - direct io: queue not effectively used

#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int o_direct = 0;
#ifdef __linux__
    if (argc > 1 && strcasecmp(argv[1], "direct") == 0) {
        o_direct = O_DIRECT;
        printf("enable direct io\n");
    }
#endif

    int fd = open("./__test__.bin", O_CREAT | O_TRUNC | O_WRONLY | o_direct, 0644);
    if (fd < 0) { perror("open"); return 1; }

    const int size = 10 * 1024;
    char buf[size];
    memset(buf, 0, size);

    for (int i = 0; i < 1024 * 1024; ++i) {
        if (write(fd, buf, size) != size) {
            perror("write"); return 2;
        }
    }
    close(fd);

    return 0;
}
