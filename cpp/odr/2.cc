#include "base.h"

// `Derived` re-defined in 1.cc
struct Derived : Base {
    int f() override { return 2; }
};

// may return `Derived` defined in 1.cc, not here
Base* get2() { return new Derived; }
