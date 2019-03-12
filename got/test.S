.text

.global test_asm

test_asm:
#if 1
    adrp    x9, :got:test_c
    ldr     x9, [x9, :got_lo12:test_c]
#else
    adrp    x9, test_c
    add     x9, x9, :lo12:test_c
#endif

    br      x9
