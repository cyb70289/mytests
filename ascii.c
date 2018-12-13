#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <arm_neon.h>

int ascii_u64_asm(const uint8_t *data, int len);

static int ascii_u64(const uint8_t *data, int len)
{
    uint8_t orall = 0;

    if (len >= 16) {
        uint64_t or1 = 0, or2 = 0;
        const uint8_t *data2 = data+8;

        do {
            or1 |= *(const uint64_t *)data;
            or2 |= *(const uint64_t *)data2;
            data += 16;
            data2 += 16;
            len -= 16;
        } while (len >= 16);

        orall = !((or1 | or2) & 0x8080808080808080ULL) - 1;
    }

    while (len--)
        orall |= *data++;

    return orall < 0x80;
}

static int ascii_neon(const uint8_t *data, int len)
{
    if (len >= 32) {
        const uint8_t *data2 = data+16;

        uint8x16_t or1 = vdupq_n_u8(0), or2 = or1;

        while (len >= 32) {
            const uint8x16_t input1 = vld1q_u8(data);
            const uint8x16_t input2 = vld1q_u8(data2);

            or1 = vorrq_u8(or1, input1);
            or2 = vorrq_u8(or2, input2);

            data += 32;
            data2 += 32;
            len -= 32;
        }

        or1 = vorrq_u8(or1, or2);
        if (vmaxvq_u8(or1) >= 0x80)
            return 0;
    }

    return ascii_u64(data, len);
}

struct ftab {
    const char *name;
    int (*func)(const uint8_t *data, int len);
};

const struct  ftab _f[] = {
    {
        .name = "u64",
        .func = ascii_u64,
    }, {
        .name = "u64_asm",
        .func = ascii_u64_asm,
    }, {
        .name = "neon",
        .func = ascii_neon,
    },
};

static void load_test_buf(uint8_t *data, int len)
{
    uint8_t v = 0;

    for (int i = 0; i < len; ++i) {
        data[i] = v++;
        v &= 0x7F;
    }
}

static void bench(const struct ftab *f, const uint8_t *data, int len)
{
    const int loops = 1024*1024*1024/len;
    int ret = 1;
    double time_aligned, time_unaligned, size;
    struct timeval tv1, tv2;

    fprintf(stderr, "bench %s (%d bytes)... ", f->name, len);

    /* aligned */
    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i)
        ret &= f->func(data, len);
    gettimeofday(&tv2, 0);
    time_aligned = tv2.tv_usec - tv1.tv_usec;
    time_aligned = time_aligned / 1000000 + tv2.tv_sec - tv1.tv_sec;

    /* unaligned */
    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i)
        ret &= f->func(data+1, len);
    gettimeofday(&tv2, 0);
    time_unaligned = tv2.tv_usec - tv1.tv_usec;
    time_unaligned = time_unaligned / 1000000 + tv2.tv_sec - tv1.tv_sec;

    printf("%s ", ret?"pass":"FAIL");

    size = ((double)len * loops) / (1024*1024);
    printf("%.0f/%.0f MB/s\n", size / time_aligned, size / time_unaligned);
}

static void test(const struct ftab *f, uint8_t *data, int len)
{
    int error = 0;

    fprintf(stderr, "test %s (%d bytes)... ", f->name, len);

    /* positive */
    error |= !f->func(data, len);

    /* negative */
    if (len < 100*1024) {
        for (int i = 0; i < len; ++i) {
            data[i] += 0x80;
            error |= f->func(data, len);
            data[i] -= 0x80;
        }
    }

    printf("%s\n", error ? "FAIL" : "pass");
}

/* ./ascii [test|bench] [alg] */
int main(int argc, const char *argv[])
{
    int do_test = 1, do_bench = 1;
    const char *alg = NULL;

    if (argc > 1) {
        do_bench &= !!strcmp(argv[1], "test");
        do_test &= !!strcmp(argv[1], "bench");
    }

    if (do_bench && argc > 2)
        alg = argv[2];

    const int size[] = {
        9, 16+1, 32-1, 128+1, 1024+15, 16*1024+1, 64*1024+15, 1024*1024
    };
    const int size_cnt = sizeof(size) / sizeof(size[0]);

    int max_size = size[size_cnt-1];
    uint8_t *_data = malloc(max_size+1);
    uint8_t *data = _data+1;    /* Unalign buffer address */

    _data[0] = 0;
    load_test_buf(data, max_size);

    if (do_test) {
        printf("==================== Test ====================\n");
        for (int i = 0; i < size_cnt; ++i)
            for (int j = 0; j < sizeof(_f)/sizeof(_f[0]); ++j)
                test(&_f[j], data, size[i]);
    }

    if (do_bench) {
        printf("==================== Bench ====================\n");
        for (int i = 0; i < size_cnt; ++i) {
            for (int j = 0; j < sizeof(_f)/sizeof(_f[0]); ++j)
                if (!alg || strcmp(alg, _f[j].name) == 0)
                    bench(&_f[j], _data, size[i]);
            printf("-----------------------------------------------\n");
        }
    }

    free(_data);
    return 0;
}
