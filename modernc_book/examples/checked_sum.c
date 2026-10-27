#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>

static bool sum_nonnegative(const unsigned *items, size_t count, unsigned *out)
{
    if (out == nullptr || (count != 0 && items == nullptr))
        return false;
    unsigned total = 0;
    for (size_t i = 0; i < count; ++i) {
        if (items[i] > UINT_MAX - total) return false;
        total += items[i];
    }
    *out = total;
    return true;
}

int main(void)
{
    const unsigned good[] = {1, 2, 3};
    const unsigned bad[] = {UINT_MAX, 1};
    unsigned result = 47;
    if (!sum_nonnegative(good, 3, &result) || result != 6) return 1;
    if (sum_nonnegative(bad, 2, &result) || result != 6) return 1;
    if (!sum_nonnegative(nullptr, 0, &result) || result != 0) return 1;
    puts("checked_sum: OK");
    return 0;
}
