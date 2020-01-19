#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static unsigned ceph_str_hash_linux_new(char *str, unsigned length)
{
    unsigned hash = 0;    /* long removed */

    while (length--) {
        unsigned char c = *str++; 
        hash = (hash + (c << 4) + (c >> 4)) * 11;
    }
    return hash;
}

static unsigned ceph_str_hash_linux_old(char *str, unsigned length)
{
    unsigned long hash = 0;

    while (length--) {
        unsigned char c = *str++; 
        hash = (hash + (c << 4) + (c >> 4)) * 11;
    }
    return hash;
}

int main(void)
{
    const int block_size[] = { 64, 4096, 65536, 1024*1024 };
    const int block_cnt = sizeof(block_size) / sizeof(block_size[0]);

    char *str = malloc(block_size[block_cnt-1]);
    srand(123456);
    for (int i = 0; i < block_size[block_cnt-1]; i++)
        str[i] = rand();

    unsigned s;
    struct timeval tv1, tv2;
    double tnew, told;

    for (int l = 0; l < block_cnt; l++) {
        int loop_count = (1U << 29) / block_size[l];

        s = 0;
        /* warm up */
        for (int i = 0; i < loop_count; i++)
            s += ceph_str_hash_linux_new(str, block_size[l]);

        gettimeofday(&tv1, 0);
        for (int i = 0; i < loop_count; i++)
            s += ceph_str_hash_linux_new(str, block_size[l]);
        gettimeofday(&tv2, 0);
        tnew = tv2.tv_usec - tv1.tv_usec;
        tnew = tnew / 1000000 + tv2.tv_sec - tv1.tv_sec;
        printf("time-new = %.2f s, hash = %x\n", tnew, s);

        s = 0;
        /* warm up */
        for (int i = 0; i < loop_count; i++)
            s += ceph_str_hash_linux_old(str, block_size[l]);

        gettimeofday(&tv1, 0);
        for (int i = 0; i < loop_count; i++)
            s += ceph_str_hash_linux_old(str, block_size[l]);
        gettimeofday(&tv2, 0);
        told = tv2.tv_usec - tv1.tv_usec;
        told = told / 1000000 + tv2.tv_sec - tv1.tv_sec;
        printf("time-old = %.2f s, hash = %x\n", told, s);

        printf("Improves [%.2f%%] when block_size = %d\n\n",
                (told - tnew) * 100.f / told,
                block_size[l]);
    }

    return 0;
}
