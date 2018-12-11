#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

#include <sys/time.h>

template <typename T>
static inline T read_le_char(const char* p)
{
    T datum;
    std::copy_n(p, sizeof(T), reinterpret_cast<char*>(&datum));
    return datum;
}

template <typename T>
static inline T read_le_uchar(const unsigned char* p)
{
    T datum;
    std::copy_n(p, sizeof(T), reinterpret_cast</*unsigned*/ char*>(&datum));
    return datum;
}

static uint8_t bench_char_char(const uint8_t *data, int len)
{
    uint8_t orall = 0;

    if (len >= 16) {
        uint64_t or1 = 0, or2 = 0;
        const uint8_t *data2 = data+8;

        while (len >= 16) {
            /////////////////////////////////////////////
            or1 |= read_le_char<uint64_t>((char *)data);
            or2 |= read_le_char<uint64_t>((char *)data2);
            /////////////////////////////////////////////
            data += 16;
            data2 += 16;
            len -= 16;
        }

        union {
            uint64_t u64;
            uint8_t u8[8];
        } orr;

        orr.u64 = or1 | or2;
        for (int i = 0; i < 8; ++i)
            orall |= orr.u8[i];
    }

    while (len--)
        orall |= *data++;

    return orall;
}

static uint8_t bench_uchar_char(const uint8_t *data, int len)
{
    uint8_t orall = 0;

    if (len >= 16) {
        uint64_t or1 = 0, or2 = 0;
        const uint8_t *data2 = data+8;

        while (len >= 16) {
            ///////////////////////////////////////////////////////
            or1 |= read_le_uchar<uint64_t>((unsigned char *)data);
            or2 |= read_le_uchar<uint64_t>((unsigned char *)data2);
            ///////////////////////////////////////////////////////
            data += 16;
            data2 += 16;
            len -= 16;
        }

        union {
            uint64_t u64;
            uint8_t u8[8];
        } orr;

        orr.u64 = or1 | or2;
        for (int i = 0; i < 8; ++i)
            orall |= orr.u8[i];
    }

    while (len--)
        orall |= *data++;

    return orall;
}

static int bench(uint8_t (*func)(const uint8_t *, int), const char *name,
        uint8_t *data, int len)
{
    const int loops = 1024*1024*1024/len;
    uint8_t ret = 0;
    double time_unaligned, size;
    struct timeval tv1, tv2;

    fprintf(stderr, "bench %s (%d bytes)... ", name, len);

    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i)
        ret |= func(data, len);
    gettimeofday(&tv2, 0);
    time_unaligned = tv2.tv_usec - tv1.tv_usec;
    time_unaligned = time_unaligned / 1000000 + tv2.tv_sec - tv1.tv_sec;

    printf("%s ", ret < 0x80 ? "pass":"FAIL");

    size = ((double)len * loops) / (1024*1024);
    printf("%.0f MB/s\n", size / time_unaligned);

    return ret;
}

int main(int argc, const char *argv[])
{
    const int max_size = 16*1024;

    uint8_t *data = new uint8_t[max_size+1];
    for (size_t i = 0; i < sizeof(data); ++i)
        data[i] = i & 0x7F;

    uint8_t *data_unaligned = data+1;    /* Unalign buffer address */

    int ret = 0;
    ret |= bench(bench_char_char, "char-char", data_unaligned, max_size);
    ret |= bench(bench_uchar_char, "uchar-char", data_unaligned, max_size);

    delete data;
    return ret;
}
