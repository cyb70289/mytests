#include <nmmintrin.h>
#include <stdio.h>

int main(void) {
    __m128i filter = _mm_set_epi8('A', 'B', 'C', 'D', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B');

    char buf[16] = {'0', '1', '2', 'B', 'A', '5', '6', 'D',
                    '0', '1', '2', '3', '4', '5', '6', 'C'};

    __m128i data = _mm_lddqu_si128((const __m128i*)buf);
    int idx = _mm_cmpistri(filter, data, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
    printf("%d\n", idx);
    return 0;
}
