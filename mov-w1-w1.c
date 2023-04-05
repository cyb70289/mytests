#include <stdio.h>

int main() {
    asm ("mov x1, 0xffffffffffffffff" : : : "x1");

    // "mov w1, w1" clears higher 32 bits of x1
    asm("mov w1, w1");

    long v;
    asm ("str x1, [%0]" : : "r"(&v));

    printf("%lx\n", v);
    return 0;
}
