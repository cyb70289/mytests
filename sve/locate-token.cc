// locate index of the first found token

#include <stdio.h>
#include <string.h>
#include <arm_sve.h>

typedef svuint8_t svuint8x16_t __attribute__((arm_sve_vector_bits(128)));
typedef svbool_t svbool16_t __attribute__((arm_sve_vector_bits(128)));

// dump vector
void dumpv(svuint8x16_t v) {
  uint8_t u[16];
  memcpy(u, &v, 16);
  for (int i = 0; i < 16; ++i) {
    printf("%02x ", u[i]);
  }
  printf("\n");
}

// dump predicate
void dumpp(svbool16_t p) {
  unsigned short u = *(unsigned short*)&p;
  printf("%04x\n", u);
}

// locate index of token in a vector
// return 0 - 15 if token is found, 16 otherwise
uint64_t locate_token() {
    // token is 0x1f, at index 5 and 15, should return 5
    const svuint8x16_t v{0,1,2,3,4,0x1f,6,7,8,9,10,11,12,13,14,0x1f};
    const svbool16_t ptrue = svptrue_b8();
    svbool_t pmatch = svmatch(ptrue, v, svdup_n_u8(0x1f));
    pmatch = svbrkb_z(ptrue, pmatch);
    return svcntp_b8(ptrue, pmatch);
}

int main() {
    printf("%d\n", (int)locate_token());
    return 0;
}
