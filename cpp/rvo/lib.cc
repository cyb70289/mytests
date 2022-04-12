#include "lib.h"
#include "stdio.h"
#include <utility>

A::A() { printf("default constructor\n"); }
A::A(const A&) { printf("copy constructor\n"); }

A f() {
  A a;
#if 1
  // triggers RVO even if caller is not in same TU
  return a;
#else
  // no RVO
  return std::move(a);
#endif
}
