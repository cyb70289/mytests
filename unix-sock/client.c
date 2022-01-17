// https://openforums.blog/2016/08/07/open-file-descriptor-passing-over-unix-domain-sockets/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

static
void send_fd(int socket, int *fds, int n)  // send fd by socket
{
        struct msghdr msg = {0};
        struct cmsghdr *cmsg;
        char buf[CMSG_SPACE(n * sizeof(int))], dup[256];
        memset(buf, '\0', sizeof(buf));
        struct iovec io = { .iov_base = dup, .iov_len = sizeof(dup) };
        struct sockaddr_un addr;

        for (int i = 0; i < sizeof(dup); ++i) {
            dup[i] = (char)i;
        }

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/fd-pass-server.socket", sizeof(addr.sun_path) - 1);

        msg.msg_name = &addr;
        msg.msg_namelen = sizeof(addr);
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        msg.msg_control = buf;
        msg.msg_controllen = sizeof(buf);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(n * sizeof(int));

        memcpy ((int *) CMSG_DATA(cmsg), fds, n * sizeof (int));

        if (sendmsg (socket, &msg, 0) < 0)
                handle_error ("Failed to send message");
}

static
void recv_msg(int socket) {
        struct msghdr msg = {0};
        char dup[256];
        struct iovec io = { .iov_base = dup, .iov_len = sizeof(dup) };

        msg.msg_iov = &io;
        msg.msg_iovlen = 1;

        if (recvmsg (socket, &msg, 0) < 0)
                handle_error ("Failed to receive message");

        for (int i = 0; i < sizeof(dup); ++i) {
            if (dup[i] != (char)i) {
                printf("ERROR! %d != %d\n", i, (int)dup[i]);
                exit(1);
            }
        }
        printf("OK!\n");
}

int
main(int argc, char *argv[]) {
        int sfd, fds[2];
        struct sockaddr_un addr;

        if (argc != 3) {
                fprintf (stderr, "Usage: %s <file-name1> <file-name2>\n", argv[0]);
                exit (1);
        }

        sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sfd == -1)
                handle_error ("Failed to create socket");

        if (unlink ("/tmp/fd-pass-client.socket") == -1 && errno != ENOENT)
                handle_error ("Removing socket file failed");

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/fd-pass-client.socket", sizeof(addr.sun_path) - 1);

        if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
                handle_error ("Failed to bind to socket");

        fds[0] = open(argv[1], O_RDONLY);
        if (fds[0] < 0)
                handle_error ("Failed to open file 1 for reading");
        else
                fprintf (stdout, "Opened fd %d in parent\n", fds[0]);

        fds[1] = open(argv[2], O_RDONLY);
        if (fds[1] < 0)
                handle_error ("Failed to open file 2 for reading");
        else
                fprintf (stdout, "Opened fd %d in parent\n", fds[1]);

        send_fd (sfd, fds, 2);
        recv_msg(sfd);

        exit(EXIT_SUCCESS);
}
