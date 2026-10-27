#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static bool fits(size_t offset, size_t count, size_t capacity)
{
    return offset <= capacity && count <= capacity - offset;
}

int main(void)
{
    const struct {
        size_t offset, count, capacity;
        bool expected;
    } tests[] = {
        {0, 0, 0, true},
        {0, 1, 0, false},
        {0, 4, 4, true},
        {1, 3, 4, true},
        {2, 3, 4, false},
        {SIZE_MAX, 1, SIZE_MAX, false},
        {SIZE_MAX, 0, SIZE_MAX, true},
        {SIZE_MAX, SIZE_MAX, SIZE_MAX, false}
    };
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; ++i)
        if (fits(tests[i].offset, tests[i].count,
                 tests[i].capacity) != tests[i].expected)
            return 1;
    puts("checked_interval: OK");
    return 0;
}
