#include <stdio.h>

inline void inlined() {
  static int a;
  printf("inline: fptr=%p, dptr=%p\n", &inlined, &a);
}

static inline void static_inlined() {
  static int a;
  printf("static inline: fptr=%p, dptr=%p\n", &static_inlined, &a);
}
