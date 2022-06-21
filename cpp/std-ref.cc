#include <stdio.h>
#include <functional>

void f(int& a) {
  a = 100;
}

template <typename T>
void uf(T a) {
  f(a);
}

int main(void) {
  int a = 0;
  // a changed with std::ref
  uf(std::ref(a));
  // a not changed without std::ref
  //uf(a);
  printf("%d\n", a);
  return 0;
}
