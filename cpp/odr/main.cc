#include "base.h"
#include <iostream>

int main() {
    Base* d1 = get1();
    Base* d2 = get2();

    // both call `Derived::f()` defined in 1.cc
    std::cout << d1->f() << std::endl;
    std::cout << d2->f() << std::endl;

    delete d1;
    delete d2;
    return 0;
}
