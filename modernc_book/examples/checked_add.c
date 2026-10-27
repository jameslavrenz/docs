#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

static bool checked_add(int a, int b, int *result)
{
    if (result == nullptr) return false;
    if ((b > 0 && a > INT_MAX - b) ||
        (b < 0 && a < INT_MIN - b))
        return false;
    *result = a + b;
    return true;
}

int main(void)
{
    int result = 0;
    if (!checked_add(3, 4, &result) || result != 7) return 1;
    if (checked_add(INT_MAX, 1, &result)) return 1;
    if (checked_add(INT_MIN, -1, &result)) return 1;
    if (!checked_add(INT_MIN, INT_MAX, &result) || result != -1)
        return 1;
    puts("checked_add: OK");
    return 0;
}
