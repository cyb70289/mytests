/*
 * Unix domain socket
 * - abstract name: "\0name"
 * - dgram receiver may truncate message
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>

#define SERVER_PATH "myserver"
#define CLIENT_PATH "myclient"

int main(int argc)
{
    int s, ret;
    char buf[16];
    struct sockaddr_un server_addr;

    s = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert(s != -1);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path+1, SERVER_PATH,
            sizeof(server_addr.sun_path)-2);

    if (argc == 1) {
        /* server */
        ret = bind(s, (struct sockaddr *)&server_addr,
                sizeof(sa_family_t)+strlen(SERVER_PATH)+1);
        assert(ret == 0);
        /* start client */
        if (fork() == 0) {
            close(s);
            execl("./sock-un", "sock-un", "client", (char *)NULL);
            assert(0);
        }

        while ((ret = read(s, buf, 8)) > 0) {
            write(STDOUT_FILENO, buf, ret);
            write(STDOUT_FILENO, "\n", 1);
        }
        wait(NULL);
        printf("server done\n");
    } else {
        /* client */
        struct sockaddr_un client_addr;

        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;
        strncpy(client_addr.sun_path+1, CLIENT_PATH,
                sizeof(client_addr.sun_path)-2);

        ret = bind(s, (struct sockaddr *)&client_addr,
                sizeof(sa_family_t)+strlen(CLIENT_PATH)+1);
        assert(ret == 0);

        ret = connect(s, (struct sockaddr *)&server_addr,
                sizeof(sa_family_t)+strlen(SERVER_PATH)+1);
        assert(ret == 0);

        for (int i = 1; i <= 16; ++i) {
            for (int j = 0; j < i; ++j) {
                buf[j] = 'a'+i-1;
            }
            write(s, buf, i);
        }
        write(s, NULL, 0);
        printf("client done\n");
    }

    close(s);
    return 0;
}
