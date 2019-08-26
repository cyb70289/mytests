/* syslog can go to stderr, must go to /var/log/system */

#include <syslog.h>

int main(int argc, char *argv[])
{
    openlog(argv[0], LOG_PERROR, LOG_USER);
    syslog(LOG_ERR, "EEEEEEEEEE");

    return 0;
}

