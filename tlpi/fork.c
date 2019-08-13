/* n forks lead to 2^n processes */

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    fork();
    fork();
    fork();
    fork();
    printf("DONE\n");

    return 0;
}
