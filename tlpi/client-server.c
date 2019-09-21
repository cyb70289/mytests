/* client & server
 *
 * client send cmds to server, server run cmds, send back response to client.
 * cmds: pwd, cd, ls, quit
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>

/* Set fd as NONBLOCK */
static void set_nonblock(int fd)
{
    int flags, ret;

    flags = fcntl(fd, F_GETFL, 0);
    assert(flags != -1);

    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    assert(ret != -1);
}

/* Write n bytes to fd */
static int writen(int fd, const void *buf, int len)
{
    int ret, written = 0;

    while (len) {
        ret = write(fd, buf, len);

        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret <= 0) {
            /* EAGAIN treated as normal error */
            break;
        }

        len -= ret;
        buf += ret;
        written += ret;
    }

    return written;
}

/* Client */
static void client(int s)
{
    int ret, err = 0;
    char buf[128], *pbuf;
    struct pollfd fds[2];

    set_nonblock(STDIN_FILENO);
    set_nonblock(s);

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN | POLLHUP;
    fds[1].fd = s;
    fds[1].events = POLLIN | POLLHUP;

    while (!err) {
        ret = poll(fds, 2, -1);
        if (ret == -1 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        } else if (ret <= 0) {
            perror("client poll");
            break;
        }

        if ((fds[0].revents & (POLLHUP|POLLERR)) ||
            (fds[1].revents & (POLLHUP|POLLERR))) {
            printf("client error\n");
            break;
        }

        if (fds[0].revents & POLLIN) {
            while (1) {
                /* read from stdin */
                ret = read(fds[0].fd, buf, 128);
                if (ret == -1 && errno == EINTR) {
                    continue;
                } else if (ret == -1 && errno == EAGAIN) {
                    break;
                } else if (ret <= 0) {
                    perror("client read stdio");
                    err = 1;
                    break;
                }

                /* write to socket */
                if (writen(fds[1].fd, buf, ret) != ret) {
                    err = 1;
                    perror("client writer socket");
                    break;
                }
            }
        }

        if (fds[1].revents & POLLIN) {
            while(1) {
                /* read from socket */
                ret = read(fds[1].fd, buf, 128);
                if (ret == -1 && errno == EINTR) {
                    continue;
                } else if (ret == -1 && errno == EAGAIN) {
                    break;
                } else if (ret <= 0) {
                    perror("client read socket");
                    err = 1;
                    break;
                }

                /* write to stdout */
                if (writen(STDOUT_FILENO, buf, ret) != ret) {
                    perror("client write stdout");
                    err = 1;
                    break;
                }
            }
        }
    }
}

static void server_pwd(int s)
{
    char *cwd = getcwd(NULL, 0);

    printf("< %s\n", cwd);
    writen(s, cwd, strlen(cwd));
    writen(s, "\n", 1);
    free(cwd);
}

static void server_cd(char *path, int s)
{
    while (*path == ' ') {
        ++path;
    }
    if (chdir(path) == 0) {
        server_pwd(s);
    } else {
        const char *r = "invalid path\n";
        printf("< %s", r);
        writen(s, r, strlen(r));
    }
}

static void server_ls(int s)
{
    size_t ret;
    FILE *fp;
    char buf[128];

    fp = popen("ls", "r");
    assert(fp);

    while ((ret = fread(buf, 1, 128, fp))) {
        writen(s, buf, ret);
    }

    fclose(fp);
}

/* Server */
static void server(int s)
{
    int ret;
    char buf[128];
    char *home_path = getcwd(NULL, 0);

    while (1) {
        /* TODO: make sure one full command line is received */
        ret = read(s, buf, 127);

        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret <= 0) {
            printf("! client gone\n");
            break;
        }

        printf("> %.*s", ret, buf);
        /* truncate trailing \r, \n, space, tab */
        buf[ret] = '\0';
        while (ret) {
            if (buf[ret-1] == '\n' || buf[ret-1] == '\r' ||
                buf[ret-1] == '\t' || buf[ret-1] == ' ') {
                buf[ret-1] = '\0';
                --ret;
            } else {
                break;
            }
        }

        if (strcmp(buf, "pwd") == 0) {
            server_pwd(s);
        } else if (strcmp(buf, "cd") == 0) {
            server_cd(home_path, s);
        } else if (strncmp(buf, "cd ", 3) == 0) {
            server_cd(buf+3, s);
        } else if (strcmp(buf, "ls") == 0) {
            server_ls(s);
        } else if (strcmp(buf, "quit") == 0) {
            close(s);
            break;
        } else {
            const char *r = "unknown cmd\n";
            printf("< %s", r);
            write(s, r, strlen(r));
        }
    }

    free(home_path);
}

int main(int argc)
{
    int s, s2, ret;
    struct addrinfo *ai;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_NUMERICSERV,
    };
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in sa;
    int sa_len = sizeof(struct sockaddr_in);

    s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s >= 0);

    if (argc == 2) {
        /* client */
        ret = getaddrinfo("127.0.0.1", "8816", &hints, &ai);
        assert(ret == 0);

        ret = connect(s, ai->ai_addr, ai->ai_addrlen);
        if (ret) {
            perror("connect");
            return 1;
        }

        client(s);
        close(s);

        return 0;
    }

    /* server */
    ret = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));

    hints.ai_flags |= AI_PASSIVE;
    ret = getaddrinfo(NULL, "8816", &hints, &ai);
    assert(ret == 0);

    ret = bind(s, ai->ai_addr, ai->ai_addrlen);
    if (ret) {
        perror("bind");
        return 1;
    }

    ret = listen(s, 5);
    assert(ret == 0);

    printf("wait for connection...\n");
    s2 = accept(s, (struct sockaddr *)&sa, &sa_len);
    assert(s2 >= 0);
    close(s);

    inet_ntop(AF_INET, &sa.sin_addr, ip, INET_ADDRSTRLEN);
    printf("connection from %s\n", ip);

    server(s2);
    close(s2);

    return 0;
}
