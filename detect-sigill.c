#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include <arm_sve.h>

signed char buf[100];
static sigjmp_buf jmpbuf;

static void sigill_handler(int) {
    siglongjmp(jmpbuf, -1);
}

__attribute__((target("+sve")))
int main () {
    signal(SIGILL, sigill_handler);

    // returns 0 if called normally, returns -1 if called by siglongjmp
    if (sigsetjmp(jmpbuf, -1) == 0) {
        // exec sve instructions
        svst1(svptrue_b8(), buf, svdup_n_s8(0x11));
        printf("no illegal instruction\n");
    } else {
        printf("captured illegal instruction\n");
    }
    signal(SIGILL, SIG_DFL);

    printf("gracely ended\n");
    return 0;
}
