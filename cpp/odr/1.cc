#include "base.h"

struct Derived : Base {
    int f() override { return 1; }
};

Base* get1() { return new Derived; }
