#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define SZ  (32*1024*1024)

int main(void)
{
    int *dd, *d[20], r = 0;

#if 0
	/* only trigger problem when 64k page size is enabeld. malloc is
       smart enough to offset on page for successive allocations to
       avoid cache contentions.
     */
    for (int i = 0; i < 20; ++i) {
        d[i] = memalign(64, 4*SZ);
        memset(d[i], 0, 4*SZ);
        printf("%p\n", d[i]);
    }
#endif

    dd = memalign(64, sizeof(int)*SZ*20);
    memset(dd, 0, sizeof(int)*SZ*20);

    /* all *d[i] share same cache line */
    for (int i = 0; i < 20; ++i)
        d[i] = dd + i*SZ;

#if 0
    /* cache line contention causes huge performance drop
     *
     * -> line 1   +-> 1   +-> 1                         1
     *         .   |   .   |   .                         .
     *         |   |   |   |   | ....................... |
     *         v   |   v   |   v                         v
     *         .   |   .   |   .                         .
     *    line 20->+   20->+   20                        20
     */
    for (int j = 0; j < SZ; ++j)
        for (int i = 0; i < 20; ++i)
            r += d[i][j];
#else
    /* tremendous performance gain by restricting to 8 ways
     *
     *  -> line 1   +-> 1   +-> 1                        1
     *          .   |   .   |   .                        .
     *          |   |   |   |   | ...................... |
     *          v   |   v   |   v                        v
     *          .   |   .   |   .                        .
     *     line 8 ->+   8 ->+   8                        8
     *                                                   |
     * +------------------------<------------------------+
     * |
     * +-> line 9   +-> 9   +-> 9                        9
     *          .   |   .   |   .                        .
     *          |   |   |   |   | ...................... |
     *          v   |   v   |   v                        v
     *          .   |   .   |   .                        .
     *     line 16->+   16->+   16                       16
     *                                                   |
     * +------------------------<------------------------+
     * |
     * +-> line 17  +-> 17  +-> 17                       17
     *          .   |   .   |   .                        .
     *          |   |   |   |   | ...................... |
     *          v   |   v   |   v                        v
     *          .   |   .   |   .                        .
     *     line 20->+   20->+   20                       20
     */
    for (int j = 0; j < SZ; ++j)
        for (int i = 0; i < 8; ++i)
            r += d[i][j];

    for (int j = 0; j < SZ; ++j)
        for (int i = 8; i < 16; ++i)
            r += d[i][j];

    for (int j = 0; j < SZ; ++j)
        for (int i = 16; i < 20; ++i)
            r += d[i][j];
#endif

    return r;
}
