#include <stdio.h>

int main(void)
{
#if 0
    /* correct for small numbers */
    unsigned int  A    = 0x567858;
    unsigned long B    = 0x5566778;
    unsigned long y0   = 0x98765ef;
    unsigned char x[2] = { 0x29, 0x17 };
#else
    /* error for big numbres due to overflow */
    unsigned int  A    = 0x56785678;
    unsigned long B    = 0x5566778811223344;
    unsigned long y0   = 0x9876543210abcdef;
    unsigned char x[2] = { 0x99, 0xE7 };
#endif

    /* precalculate A % B, A^2 % B */
    unsigned long A1 = A, A2 = A;
    A1 = A1 % B;
    A2 = (A2 * A2) % B;

    /* calculate in two steps */
    unsigned long y1 = (y0 * A + x[0]) % B;
    unsigned long y2 = (y1 * A + x[1]) % B;

    /* calculate in one step, save one division */
    unsigned long y2p = (A2 * y0 + A1 * x[0] + x[1]) % B;

    /* compare results */
    printf("%lu %lu %s\n", y2, y2p, y2 == y2p ? "ok" : "wrong");

    return 0;
}
