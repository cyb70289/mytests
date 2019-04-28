/* Compile: gcc -O3 bench-inline.c -o bench-inline */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

static int encode_utf8(unsigned long u, unsigned char *buf)
{
    if (u <= 0x0000007F) {
        buf[0] = u;
        return 1;
    } else if (u <= 0x000007FF) {
        buf[0] = 0xC0 | (u >> 6);
        buf[1] = 0x80 | (u & 0x3F);
        return 2;
    } else if (u <= 0x0000FFFF) {
        buf[0] = 0xE0 | (u >> 12);
        buf[1] = 0x80 | ((u >> 6) & 0x3F);
        buf[2] = 0x80 | (u & 0x3F);
        return 3;
    } else if (u <= 0x001FFFFF) {
        buf[0] = 0xF0 | (u >> 18);
        buf[1] = 0x80 | ((u >> 12) & 0x3F);
        buf[2] = 0x80 | ((u >> 6) & 0x3F);
        buf[3] = 0x80 | (u & 0x3F);
        return 4;
    } else {
        /* rare/illegal code points */
        if (u <= 0x03FFFFFF) {
            for (int i = 4; i >= 1; --i) {
                buf[i] = 0x80 | (u & 0x3F);
                u >>= 6;
            }
            buf[0] = 0xF8 | u;
            return 5;
        } else if (u <= 0x7FFFFFFF) {
            for (int i = 5; i >= 1; --i) {
                buf[i] = 0x80 | (u & 0x3F);
                u >>= 6;
            }
            buf[0] = 0xFC | u;
            return 6;
        }
        return -1;
    }
}

static int encode_utf8_inline(unsigned long u, unsigned char *buf)
{
    return encode_utf8(u, buf);
}

__attribute__ ((noinline))
static int encode_utf8_noinline(unsigned long u, unsigned char *buf)
{
    return encode_utf8(u, buf);
}

static void bench(unsigned long *decoded, int n_decoded,
        unsigned char *encoded, int n_encoded)
{
    const int loops = 1024*1024*1024/n_decoded;
    double time, size;
    struct timeval tv1, tv2;

    printf("bench inline...\n");

    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i) {
        unsigned char *encoded2 = encoded;
        for (int i = 0; i < n_decoded; ++i)
            encoded2 += encode_utf8_inline(decoded[i], encoded2);
    }
    gettimeofday(&tv2, 0);

    time = tv2.tv_usec - tv1.tv_usec;
    time = time / 1000000 + tv2.tv_sec - tv1.tv_sec;
    size = ((double)n_decoded * loops) / (1024*1024);
    printf("time: %.4f s\n", time);
    printf("data: %.0f Mchars\n", size);
    printf("BW: %.2f Mchars/s\n", size / time);

    /**************************************************************/

    printf("\nbench noinline...\n");

    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i) {
        unsigned char *encoded2 = encoded;
        for (int i = 0; i < n_decoded; ++i)
            encoded2 += encode_utf8_noinline(decoded[i], encoded2);
    }
    gettimeofday(&tv2, 0);

    time = tv2.tv_usec - tv1.tv_usec;
    time = time / 1000000 + tv2.tv_sec - tv1.tv_sec;
    printf("time: %.4f s\n", time);
    printf("data: %.0f Mchars\n", size);
    printf("BW: %.2f Mchars/s\n", size / time);
}

static unsigned long *load_test_file()
{
    unsigned long *data;
    int fd;
    struct stat stat;

    fd = open("./utf8-decoded.data", O_RDONLY);
    fstat(fd, &stat);
    data = malloc(stat.st_size);
    assert(read(fd, data, stat.st_size) == stat.st_size);
    close(fd);

    return data;
}

int main(int argc, char *argv[])
{
    unsigned long *decoded;
    unsigned char encoded[4096];

    decoded = load_test_file();
    bench(decoded, 1642, encoded, 4096);

    return 0;
}
