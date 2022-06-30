#include "inline.h"

void call_inlined_1();
void call_inlined_2();
void call_static_inlined_1();
void call_static_inlined_2();

int main() {
    printf("inline: one instance for all TUs\n");
    call_inlined_1();
    call_inlined_2();
    printf("\nstatic inline: each TU has its own instance\n");
    call_static_inlined_1();
    call_static_inlined_2();
    return 0;
}
