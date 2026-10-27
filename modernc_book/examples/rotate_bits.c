#include <stdint.h>
#include <limits.h>
#include <stdio.h>

/* The rotate count is interpreted modulo 32. */
static uint32_t rotate_left_32(uint32_t value, unsigned count)
{
    unsigned shift = count % 32u;
    if (shift == 0)
        return value;
    return (value << shift) | (value >> (32u - shift));
}

static uint32_t rotate_right_32(uint32_t value, unsigned count)
{
    unsigned shift = count % 32u;
    if (shift == 0)
        return value;
    return (value >> shift) | (value << (32u - shift));
}

int main(void)
{
    const uint32_t x = UINT32_C(0x12345678);
    if (rotate_left_32(x, 0) != x ||
        rotate_left_32(x, 32) != x ||
        rotate_left_32(x, 4) != UINT32_C(0x23456781) ||
        rotate_right_32(x, 4) != UINT32_C(0x81234567))
        return 1;

    for (unsigned shift = 0; shift < 64; ++shift)
        if (rotate_right_32(rotate_left_32(x, shift), shift) != x)
            return 1;

    /* All shift counts stay in [1,31] after the explicit zero case. */
    puts("rotate_bits: OK");
    return 0;
}
