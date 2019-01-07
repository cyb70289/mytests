#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define WAYS    20  /* larger than 8 (# ways of cache set) may cause trouble */
#define SIZE    (32*1024*1024)

int main(void)
{
    char *line[WAYS], r = 0;

    /* 64K page: d[i],d[i+1] differ by 32M+64K (0x02010000) */
    /* 4K  page: d[i],d[i+1] differ by 32M+4K  (0x02001000) */
    for (int i = 0; i < WAYS; ++i) {
        line[i] = memalign(64, SIZE);
        memset(line[i], 0, SIZE);
        printf("%p\n", line[i]);
    }

    /* Visit memory in columns
     *
     * Serious cache line contention if 64K page size enabled
     * - 64K page: > 20s
     * - 4K  page: < 1s
     */
    for (int j = 0; j < SIZE; ++j)
        for (int i = 0; i < WAYS; ++i)
            r += line[i][j];

    return r;
}
