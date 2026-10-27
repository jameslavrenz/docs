/* C11 and later: _Generic dispatches to typed functions at compile time. */
#include <limits.h>
#include <stdio.h>

static int maximum_int(int a, int b)
{
    return a > b ? a : b;
}

static long maximum_long(long a, long b)
{
    return a > b ? a : b;
}

/* Intended for arguments of the same supported type.
   The controlling expression (a) is not evaluated by _Generic. */
#define max_same_type(a, b) \
    _Generic((a), int: maximum_int, long: maximum_long)((a), (b))

static_assert(_Generic((int)0, int: 1, default: 0));
static_assert(_Generic((long)0, long: 1, default: 0));

int main(void)
{
    if (max_same_type(INT_MIN, INT_MAX) != INT_MAX)
        return 1;
    if (max_same_type(LONG_MIN, LONG_MAX) != LONG_MAX)
        return 1;

    int side_effect = 0;
    int chosen = max_same_type(++side_effect, 7);
    if (chosen != 7 || side_effect != 1)
        return 1;

    long a = -30;
    long b = -10;
    if (max_same_type(a, b) != -10)
        return 1;

    puts("generic_select: OK");
    return 0;
}
