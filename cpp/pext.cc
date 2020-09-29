/*
 * XXX: will crash if cpu doesn't support bmi2 pext instruction
 *
 * $ g++ -O3 -march=native -o pext pext.cc
 */

#include <iostream>
#include <chrono>
#include <cstring>

#include <immintrin.h>

// 8 bits gives best performance on my machine
static constexpr int kLookupBits = 6;
using table_type = uint8_t;

static_assert(kLookupBits <= sizeof(table_type) * 8,
              "change table type to match lookup bits");

static table_type pext_table[1 << kLookupBits][1 << kLookupBits];
static void gen_pext_lookup_table(void)
{
    for (int mask = 0; mask < (1 << kLookupBits); ++mask) {
        for (int data = 0; data < (1 << kLookupBits); ++data) {
            int bit_value = 0, bit_len = 0;
            for (int i = 0; i < kLookupBits; ++i) {
                if (mask & (1 << i)) {
                    bit_value |= (((data >> i) & 1) << bit_len);
                    ++bit_len;
                }
            }
            pext_table[mask][data] = bit_value;
        }
    }
}

static inline void pext(const uint64_t mask[], const uint64_t data[],
                        uint64_t out[], long n)
{
    for (long i = 0; i < n; ++i) {
        out[i] = _pext_u64(data[i], mask[i]);
    }
}

static inline uint64_t pext_u64_simu(uint64_t mask, uint64_t data)
{
    constexpr int kLookupMask = (1 << kLookupBits) - 1;
    uint64_t out = 0;
    int out_len = 0;

    // fixed loop performs much better for random masks
    // and no worse if mask has only few lower bits set
#if 0
    while (mask != 0) {
#else
    for (int i = 0; i < (64 + kLookupBits - 1) / kLookupBits; ++i) {
#endif
        const auto mask_value = mask & kLookupMask;
        const auto mask_len = __builtin_popcount(mask_value);
        const uint64_t value = pext_table[mask_value][data & kLookupMask];
        out |= value << out_len;
        out_len += mask_len;
        mask >>= kLookupBits;
        data >>= kLookupBits;
        if (mask == 0) {
            break;
        }
    }
    return out;
}

static inline void simu(const uint64_t mask[], const uint64_t data[],
                        uint64_t out[], long n)
{
    for (long i = 0; i < n; ++i) {
        out[i] = pext_u64_simu(mask[i], data[i]);
    }
}

static inline void dont_opt(const uint64_t out[], long n)
{
    if (out[rand() % n] == ~0ULL) {
        std::cout << "lucky" << std::endl;
    }
}

int main(void)
{
    constexpr long N = 12345678;
    constexpr int SZ = sizeof(uint64_t) * N;
    uint64_t *data = (uint64_t *)std::malloc(SZ);
    uint64_t *mask = (uint64_t *)std::malloc(SZ);
    uint64_t *out = (uint64_t *)std::malloc(SZ);

    gen_pext_lookup_table();

    /***************** prepare test data ********************/
    srand(1);
    for (long i = 0; i < N; ++i) {
        data[i] = (((uint64_t)rand()) << 32) | rand();
        mask[i] = (((uint64_t)rand()) << 32) | rand();
#if 0
        mask[i] %= 16;  // few mask bits
#endif
    }

    /****************** test correctness ******************/
    pext(mask, data, out, N);
    for (long i = 0; i < N; ++i) {
        if (pext_u64_simu(mask[i], data[i]) != out[i]) {
            std::cout << "ERROR!!!" << std::endl;
            return 1;
        }
    }

    /********************* BMI2 pext **********************/
    /* warm up */
    std::memset(out, 0, SZ);
    pext(mask, data, out, N);
    dont_opt(out, N);

    /* bench */
    std::memset(out, 0, SZ);
    auto t1 = std::chrono::high_resolution_clock::now();
    pext(mask, data, out, N);
    auto t2 = std::chrono::high_resolution_clock::now();
    dont_opt(out, N);

    std::chrono::duration<double> elapsed = t2 - t1;
    std::cout << "bmi2 pext: " << elapsed.count() << " s\n";

    /****************** simulated pext *******************/
    /* warm up */
    std::memset(out, 0, SZ);
    simu(mask, data, out, N);
    dont_opt(out, N);

    /* bench */
    std::memset(out, 0, SZ);
    t1 = std::chrono::high_resolution_clock::now();
    simu(mask, data, out, N);
    t2 = std::chrono::high_resolution_clock::now();
    dont_opt(out, N);

    elapsed = t2 - t1;
    std::cout << "simu pext: " << elapsed.count() << " s\n";

    std::free(data);
    std::free(mask);
    std::free(out);

    return 0;
}
