/* int ascii_u64_asm(const uint8_t *data, int len) */
.global ascii_u64_asm

/* Arguments */
x_data      .req    x0
w_len       .req    w1
x_len       .req    x1
/* Returns: 0-invalid, 1-valid */
w_valid     .req    w0
/* Local variables */
w_orall     .req    w2
x_or1       .req    x3
x_or2       .req    x4
w_tmp1      .req    w5
x_tmp1      .req    x5
x_tmp2      .req    x6
x_data_end  .req    x7

.text

ascii_u64_asm:
    mov     w_orall, 0
    cmp     w_len, 16
    blt     .Lremaining

    mov     x_or1, 0
    mov     x_or2, 0

    /* x_data_end = x_data + x_len - 16 */
    add     x_data_end, x_data, x_len
    sub     x_data_end, x_data_end, #16

.L16_loop:
    ldp     x_tmp1, x_tmp2, [x_data], #16
    orr     x_or1, x_or1, x_tmp1
    orr     x_or2, x_or2, x_tmp2
    cmp     x_data, x_data_end
    bls     .L16_loop

    /* w_len = w_len % 16 */
    and     w_len, w_len, #0xF

    /* w_orall = ((x_or1 | x_or2) & 0x8080808080808080) ? 0xFFFFFFFF : 0 */
    orr     x_or1, x_or1, x_or2
    tst     x_or1, #0x8080808080808080
    csetm   w_orall, ne

.Lremaining:
    cbz     w_len, .Lend

.Lremaining_loop:
    ldrb    w_tmp1, [x_data], #1
    orr     w_orall, w_orall, w_tmp1
    subs    w_len, w_len, #1
    bne     .Lremaining_loop

.Lend:
    /* w_valid = (w_orall & 0x80) ? 0 : 1 */
    sxtb    w_valid, w_orall
    mvn     w_valid, w_valid
    lsr     w_valid, w_valid, #31
    ret
