#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static bool decode_utf8(const unsigned char *s, size_t n,
                        uint32_t *codepoint, size_t *consumed)
{
    if (s == nullptr || codepoint == nullptr || consumed == nullptr || n == 0)
        return false;
    uint32_t cp;
    size_t length;
    unsigned char a = s[0];
    if (a <= 0x7f) { cp = a; length = 1; }
    else if (a >= 0xc2 && a <= 0xdf) { cp = a & 0x1f; length = 2; }
    else if (a >= 0xe0 && a <= 0xef) { cp = a & 0x0f; length = 3; }
    else if (a >= 0xf0 && a <= 0xf4) { cp = a & 0x07; length = 4; }
    else return false;
    if (n < length) return false;
    for (size_t i = 1; i < length; ++i) {
        if ((s[i] & 0xc0) != 0x80) return false;
        cp = (cp << 6) | (uint32_t)(s[i] & 0x3f);
    }
    if ((length == 2 && cp < 0x80) ||
        (length == 3 && cp < 0x800) ||
        (length == 4 && cp < 0x10000) ||
        (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff)
        return false;
    *codepoint = cp;
    *consumed = length;
    return true;
}
int main(void)
{
    const unsigned char euro[] = {0xe2, 0x82, 0xac};
    const unsigned char overlong[] = {0xc0, 0x80};
    const unsigned char surrogate[] = {0xed, 0xa0, 0x80};
    const unsigned char beyond[] = {0xf4, 0x90, 0x80, 0x80};
    uint32_t cp = 99;
    size_t count = 99;
    if (!decode_utf8(euro, sizeof euro, &cp, &count) ||
        cp != 0x20ac || count != 3) return 1;
    if (decode_utf8(overlong, sizeof overlong, &cp, &count) ||
        decode_utf8(surrogate, sizeof surrogate, &cp, &count) ||
        decode_utf8(beyond, sizeof beyond, &cp, &count) ||
        decode_utf8(euro, 2, &cp, &count)) return 1;
    if (cp != 0x20ac || count != 3) return 1;
    puts("utf8_decode: OK");
    return 0;
}
