#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

/* Exact ASCII decimal grammar: optional sign followed by >=1 digit.
   No leading spaces and no trailing text. Output unchanged on failure. */
static bool parse_long(const char *text, long *out)
{
    if (text == nullptr || out == nullptr || *text == '\0')
        return false;

    bool negative = false;
    const unsigned char *p = (const unsigned char *)text;
    if (*p == '-' || *p == '+') {
        negative = *p == '-';
        ++p;
        if (*p == '\0')
            return false;
    }

    const unsigned long limit =
        negative ? (unsigned long)LONG_MAX + 1UL : (unsigned long)LONG_MAX;
    unsigned long magnitude = 0;
    for (; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        unsigned digit = (unsigned)(*p - '0');
        if (magnitude > (limit - digit) / 10UL)
            return false;
        magnitude = 10UL * magnitude + digit;
    }

    long answer;
    if (negative && magnitude == (unsigned long)LONG_MAX + 1UL)
        answer = LONG_MIN;
    else
        answer = negative ? -(long)magnitude : (long)magnitude;
    *out = answer;
    return true;
}

int main(void)
{
    const struct { const char *text; long expected; } cases[] = {
        {"0", 0}, {"-0", 0}, {"+42", 42}, {"-17", -17},
        {"00013", 13}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        long result = 999;
        if (!parse_long(cases[i].text, &result) ||
            result != cases[i].expected)
            return 1;
    }

    char maximum[64];
    char minimum[64];
    if (snprintf(maximum, sizeof maximum, "%ld", LONG_MAX) < 0 ||
        snprintf(minimum, sizeof minimum, "%ld", LONG_MIN) < 0)
        return 1;
    long value = 0;
    if (!parse_long(maximum, &value) || value != LONG_MAX ||
        !parse_long(minimum, &value) || value != LONG_MIN)
        return 1;

    const char *invalid[] = {
        "", "+", "-", " 42", "4 ", "4z",
        "999999999999999999999999999999999",
        "-999999999999999999999999999999999"
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        value = 123;
        if (parse_long(invalid[i], &value) || value != 123)
            return 1;
    }
    puts("parse_signed: OK");
    return 0;
}
