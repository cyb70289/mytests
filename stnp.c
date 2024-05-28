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
