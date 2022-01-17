// https://openforums.blog/2016/08/07/open-file-descriptor-passing-over-unix-domain-sockets/

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

static
int * recv_fd(int socket, int n) {
        int *fds = (int*)malloc (n * sizeof(int));
        struct msghdr msg = {0};
        struct cmsghdr *cmsg;
        char buf[CMSG_SPACE(n * sizeof(int))], dup[256];
        memset(buf, '\0', sizeof(buf));
        struct iovec io = { .iov_base = dup, .iov_len = sizeof(dup) };

        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        msg.msg_control = buf;
        msg.msg_controllen = sizeof(buf);

        if (recvmsg (socket, &msg, 0) < 0)
                handle_error ("Failed to receive message");

        cmsg = CMSG_FIRSTHDR(&msg);

        memcpy (fds, (int *) CMSG_DATA(cmsg), n * sizeof(int));

        for (int i = 0; i < sizeof(dup); ++i) {
            if (dup[i] != (char)i) {
                printf("ERROR! %d != %d\n", i, (int)dup[i]);
                exit(1);
            }
        }

        return fds;
}

static
void send_msg(int socket)
{
        struct msghdr msg = {0};
        char dup[256];
        struct iovec io = { .iov_base = dup, .iov_len = sizeof(dup) };
        struct sockaddr_un addr;

        for (int i = 0; i < sizeof(dup); ++i) {
            dup[i] = (char)i;
        }

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/fd-pass-client.socket", sizeof(addr.sun_path) - 1);

        msg.msg_name = &addr;
        msg.msg_namelen = sizeof(addr);
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;

        if (sendmsg (socket, &msg, 0) < 0)
                handle_error ("Failed to send message");
}

int
main(int argc, char *argv[]) {
        ssize_t nbytes;
        char buffer[256];
        int sfd, *fds;
        struct sockaddr_un addr;

        sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sfd == -1)
                handle_error ("Failed to create socket");

        if (unlink ("/tmp/fd-pass-server.socket") == -1 && errno != ENOENT)
                handle_error ("Removing socket file failed");

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/fd-pass-server.socket", sizeof(addr.sun_path) - 1);

        if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
                handle_error ("Failed to bind to socket");

        fds = recv_fd (sfd, 2);

        for (int i=0; i<2; ++i) {
                fprintf (stdout, "Reading from passed fd %d\n", fds[i]);
                while ((nbytes = read(fds[i], buffer, sizeof(buffer))) > 0)
                        write(1, buffer, nbytes);
                *buffer = '\0';
        }

        send_msg(sfd);

        if (close(sfd) == -1)
                handle_error ("Failed to close client socket");

        return 0;
}
