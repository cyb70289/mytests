/* Send file descriptor through unix domain socket */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>

int main(int argc)
{
    int s[2], ret, fd;
    size_t len;
    struct msghdr msg;
    struct cmsghdr *cmsg;

    ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, s);
    assert(ret != -1);

    len = sizeof(struct cmsghdr) + sizeof(int);
    cmsg = malloc(len);
    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_control = cmsg;
    msg.msg_controllen = len;

    if (fork()) {
        /* parent */
        close(s[1]);
        fd = open("/tmp/__xxxxyyyy__", O_CREAT|O_WRONLY|O_TRUNC, 0666);
        assert(fd != -1);

        cmsg->cmsg_len = len;
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *(int *)CMSG_DATA(cmsg) = fd;

        ret = sendmsg(s[0], &msg, 0);
        assert(ret == 0);

        wait(NULL);
    } else {
        /* child */
        close(s[0]);

        ret = recvmsg(s[1], &msg, 0);
        assert(ret == 0);

        fd = *(int *)CMSG_DATA(cmsg);
        ret = write(fd, "CHILD", 5);
        assert(ret == 5);
    }

    close(fd);
    free(cmsg);
    return 0;
}
