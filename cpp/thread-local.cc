#include <stdio.h>

struct A {
    A() { printf("aaaaaaa\n"); }
    void f() {}
};

struct B {
    B() { printf("bbbbbbb\n"); }
};

struct C {
    C() { printf("ccccccc\n"); }
};

static thread_local A a;
static thread_local B b;

void cc() {
    static thread_local C c;
}

int main() {
#if 0
    // a, b not instantiated if not used
#else
    // only a is used, but b is also instantiated
    a.f();
#endif
    return 0;
}
