/* gcc -O3 utf8-len.c -o utf8-len */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

static inline int utf8_char_length(char c)
{
  if ((signed char)c >= 0) {  // 1-byte char (0x00 ~ 0x7F)
    return 1;
  } else if ((c & 0xE0) == 0xC0) {  // 2-byte char
    return 2;
  } else if ((c & 0xF0) == 0xE0) {  // 3-byte char
    return 3;
  } else if ((c & 0xF8) == 0xF0) {  // 4-byte char
    return 4;
  }
  // invalid char
  return 0;
}

static inline int utf8_char_length_tbl(char c)
{
    static const char tbl[32] = {
        /* 0b00000 ~ 0b01111 */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0b01111 ~ 0b10111 */
        0, 0, 0, 0, 0, 0, 0, 0,
        /* 0b11000 ~ 0b11011 */
        2, 2, 2, 2,
        /* 0b11100 ~ 0b11101 */
        3, 3,
        /* 0b11110 */
        4,
        /* 0b11111 */
        0,
    };

    return tbl[((unsigned char)c) >> 3];
}

static int utf8_length(const char* data, int data_len)
{
    int char_len = 0;
    int count = 0;
    for (int i = 0; i < data_len; i += char_len) {
        char_len = utf8_char_length(data[i]);
        if (char_len == 0 || i + char_len > data_len) {
            printf("utf8_length: ERROR\n");
            exit(1);
        }
        for (int j = 1; j < char_len; ++j) {
            if ((data[i + j] & 0xC0) != 0x80) {
                printf("utf8_length: ERROR\n");
                exit(1);
            }
        }
        ++count;
    }
    return count;
}

static int utf8_length_tbl(const char* data, int data_len)
{
    int char_len = 0;
    int count = 0;
    for (int i = 0; i < data_len; i += char_len) {
        char_len = utf8_char_length_tbl(data[i]);
        if (char_len == 0 || i + char_len > data_len) {
            printf("utf8_length_tbl: ERROR\n");
            exit(1);
        }
        for (int j = 1; j < char_len; ++j) {
            if ((data[i + j] & 0xC0) != 0x80) {
                printf("utf8_length_tbl: ERROR\n");
                exit(1);
            }
        }
        ++count;
    }
    return count;
}

static void show_speed(struct timeval *tv1, struct timeval *tv2, double size)
{
    double time = tv2->tv_usec - tv1->tv_usec;
    time = time / 1000000 + tv2->tv_sec - tv1->tv_sec;
    size /= (1024*1024);
    printf("%.2f MB/s\n", size / time);
}

int main(void)
{
    const int rounds = 5*1024*1024;
    const int bufsz = 256;

    long n = 0;
    char buf[bufsz];
    struct timeval tv1, tv2;

    /* verify that optimized code works correctly */
    for (int c = 0; c < 256; ++c) {
        if (utf8_char_length(c) != utf8_char_length_tbl(c)) {
            printf("ERROR!!!\n");
            return 1;
        }
    }

    srand(time(NULL));

    printf("********** microbenchmark with random buffer **********\n");
    for (int i = 0; i < bufsz; ++i) {
        buf[i] = rand();
    }

    printf("bench utf8_char_length: original\n");
    gettimeofday(&tv1, 0);
    for (int i = 0; i < rounds; ++i) {
        for (int j = 0; j < bufsz; ++j) {
            n += utf8_char_length(buf[j]);
        }
    }
    gettimeofday(&tv2, 0);
    show_speed(&tv1, &tv2, (double)rounds*bufsz);

    printf("bench utf8_char_length_tb: lookup table\n");
    gettimeofday(&tv1, 0);
    for (int i = 0; i < rounds; ++i) {
        for (int j = 0; j < bufsz; ++j) {
            n += utf8_char_length_tbl(buf[j]);
        }
    }
    gettimeofday(&tv2, 0);
    show_speed(&tv1, &tv2, (double)rounds*bufsz);

    printf("********** benchmark with ascii buffer **********\n");
    for (int i = 0; i < bufsz; ++i) {
        buf[i] &= 0x7F;
    }

    printf("bench utf8_length: original\n");
    gettimeofday(&tv1, 0);
    for (int i = 0; i < rounds; ++i) {
        n += utf8_length(buf, bufsz) ;
    }
    gettimeofday(&tv2, 0);
    show_speed(&tv1, &tv2, (double)rounds*bufsz);

    printf("bench utf8_length: lookup table\n");
    gettimeofday(&tv1, 0);
    for (int i = 0; i < rounds; ++i) {
        n += utf8_length_tbl(buf, bufsz);
    }
    gettimeofday(&tv2, 0);
    show_speed(&tv1, &tv2, (double)rounds*bufsz);

    return n;
}
