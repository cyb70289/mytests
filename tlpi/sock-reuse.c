/*
 * - Parent accept at port
 * - Fork child to handle the accepted socket
 * - Parent exit, leave child running
 * - Parent restart failed if not set socket reuseaddr
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(void)
{
    int s, s2, ret;
    struct addrinfo *ai;
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE | AI_NUMERICSERV,
    };
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in sa;
    int sa_len = sizeof(struct sockaddr_in);

    s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s >= 0);

    /* without this, server cannot restart when child is running */
    ret = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));

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

    inet_ntop(AF_INET, &sa.sin_addr, ip, INET_ADDRSTRLEN);
    printf("connection from %s\n", ip);
    printf("parent exit, child handling the connection\n");

    if (fork() == 0) {
        char buf[10];

        close(s);
        while (read(s2, buf, 10) > 0)
            ;
        close(s2);
        printf("child exit\n");
    }

    close(s);
    close(s2);

    return 0;
}
