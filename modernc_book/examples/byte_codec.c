#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>

#if CHAR_BIT != 8
#error "This octet-oriented example needs 8-bit bytes"
#endif

static bool encode_u32_be(uint32_t value, unsigned char *dst, size_t capacity)
{
    if (dst == nullptr || capacity < 4) return false;
    dst[0] = (unsigned char)(value >> 24);
    dst[1] = (unsigned char)(value >> 16);
    dst[2] = (unsigned char)(value >> 8);
    dst[3] = (unsigned char)value;
    return true;
}

static bool decode_u32_be(const unsigned char *src, size_t length, uint32_t *out)
{
    if (src == nullptr || out == nullptr || length < 4) return false;
    uint32_t value = ((uint32_t)src[0] << 24)
                   | ((uint32_t)src[1] << 16)
                   | ((uint32_t)src[2] << 8)
                   | (uint32_t)src[3];
    *out = value;
    return true;
}

int main(void)
{
    const uint32_t tests[] = { UINT32_C(0), UINT32_C(0x12345678), UINT32_MAX };
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; ++i) {
        unsigned char bytes[4] = {};
        uint32_t decoded = 0;
        if (!encode_u32_be(tests[i], bytes, sizeof bytes)) return 1;
        if (!decode_u32_be(bytes, sizeof bytes, &decoded)) return 1;
        if (decoded != tests[i]) return 1;
        if (encode_u32_be(tests[i], bytes, 3)) return 1;
    }
    puts("byte_codec: OK");
    return 0;
}
