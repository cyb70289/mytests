#include <arm_sve.h>
#include <iostream>

typedef svint8_t svfixed_int8_t __attribute__((arm_sve_vector_bits(128)));
typedef svbool_t svfixed_bool_t __attribute__((arm_sve_vector_bits(128)));

int main() {
  const svfixed_int8_t haystack = {
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  };
  const svfixed_int8_t needle = {
    -1, -1, -1, -1, 12, -1, 7, -1, -1, -1, -1, 12, -1, -1, -1, 9,
  };
  const svfixed_bool_t pg = svptrue_b8();

  const svfixed_bool_t p = svmatch_s8(svptrue_b8(), haystack, needle);
  const unsigned short mask = *(unsigned short*)&p;
  if (mask) {
    std::cout << __builtin_ctz(mask) << '\n';
  } else {
    std::cout << "not found\n";
  }

  return 0;
}
