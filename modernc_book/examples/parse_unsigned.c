#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

static bool parse_unsigned(const char *text, unsigned *out)
{
    if (text == nullptr || out == nullptr || *text == '\0')
        return false;
    unsigned value = 0;
    for (const unsigned char *p = (const unsigned char *)text;
         *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;
        unsigned digit = (unsigned)(*p - '0');
        if (value > (UINT_MAX - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    *out = value;
    return true;
}

int main(void)
{
    const struct { const char *text; bool valid; unsigned value; } cases[] = {
        {"0", true, 0},
        {"42", true, 42},
        {"", false, 0},
        {"-1", false, 0},
        {"12x", false, 0},
        {"999999999999999999999999", false, 0}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        unsigned result = 777;
        bool ok = parse_unsigned(cases[i].text, &result);
        if (ok != cases[i].valid || (ok && result != cases[i].value))
            return 1;
    }
    puts("parse_unsigned: OK");
    return 0;
}
