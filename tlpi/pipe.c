#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

int main(void)
{
    int ret;
    int pipe_p2c[2], pipe_c2p[2];
    char c;

    signal(SIGPIPE, SIG_IGN);

    ret = pipe(pipe_p2c);
    assert(ret == 0);
    ret = pipe(pipe_c2p);
    assert(ret == 0);

    if (fork() == 0) {
        /* child */
        close(pipe_p2c[1]);
        close(pipe_c2p[0]);
        /* echo capitalized char */
        while (1) {
            ret = read(pipe_p2c[0], &c, 1);
            if (ret != 1) {
                break;
            }
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }

            ret = write(pipe_c2p[1], &c, 1);
            if (ret != 1) {
                break;
            }
        }
        close(pipe_p2c[0]);
        close(pipe_c2p[1]);
    } else {
        /* parent */
        close(pipe_p2c[0]);
        close(pipe_c2p[1]);
        /* stdio -> pipe_p2c -> pipe_c2p -> stdout */
        while (1) {
            read(STDIN_FILENO, &c, 1);
            if (c == '\n') {
                break;
            }
            write(pipe_p2c[1], &c, 1);
            read(pipe_c2p[0], &c, 1);
            write(STDOUT_FILENO, &c, 1);
        }
        close(pipe_p2c[1]);
        close(pipe_c2p[0]);

        wait(NULL);
    }

    return 0;
}
