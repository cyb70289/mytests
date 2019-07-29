/* clock() returns cpu time, sleep() is not counted in */

#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main(void)
{
    clock_t t1, t2;

    t1 = clock();
    sleep(1);
    t2 = clock();

    printf("%lu\n", (unsigned long)(t2-t1));

    return 0;
}
