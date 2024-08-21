void my_reorder(const void *in, void *out) {
    float *outv = (float *)out;
    for (int col = 0; col < 256; col += 4) {
        const float *inv = (const float *)in + col;
        for (int row = 0; row < 128; row += 2) {
            asm volatile(
                "ldr     q0, [%[inv]]\n\t"
                "ldr     q1, [%[inv], 256*4]\n\t"
                "stnp    q0, q1, [%[outv]]"
                :
                : [outv] "r" (outv), [inv] "r" (inv)
                : "q0", "q1"
            );
            outv += 8;
            inv += 512;
        }
    }
}

// return ((vector - 31) ^ 29) - 29
uint8x16_t f2(uint8x16_t vector) {
  __asm__ __volatile__(
    "movi    v1.16b, #31\n\t"
    "movi    v2.16b, #29\n\t"
    "sub     %[vector].16b, %[vector].16b, v1.16b\n\t"
    "eor     %[vector].16b, %[vector].16b, v2.16b\n\t"
    "sub     %[vector].16b, %[vector].16b, v2.16b"
    : [vector] "=w" (vector)
    :
    : "v1", "v2"
  );
  return vector;
}

// static constexpr uint8_t tbl[33] = {
//   5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3,
//   3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
//   1
// };
// return tbl[i];
uint32_t f3(uint32_t i) {
  uint32_t ret;
  __asm__ __volatile__(
    "adr     x1, 1f\n\t"
    "b       2f\n\t"
    "1:\n\t"
    ".byte   5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3\n\t"
    ".byte   3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1\n\t"
    ".byte   1, 0, 0, 0\n\t"
    "2:\n\t"
    "ldrb    %w[ret], [x1, %[i]]"
    : [ret] "=r" (ret)
    : [i] "r" (i)
    : "x1"
  );
  return ret;
}
