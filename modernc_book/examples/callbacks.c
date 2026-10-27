#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef bool (*visitor_fn)(int, void *);

static bool visit_all(const int *items, size_t count,
                      visitor_fn visitor, void *context)
{
    if (visitor == nullptr || (count != 0 && items == nullptr)) return false;
    for (size_t i = 0; i < count; ++i)
        if (!visitor(items[i], context)) return false;
    return true;
}
struct counter { int threshold; size_t count; };
static bool count_greater(int value, void *context)
{
    struct counter *c = context;
    if (value > c->threshold) ++c->count;
    return true;
}
static int order_ints(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}
int main(void)
{
    int values[] = {INT_MAX, 4, INT_MIN, 7, 4};
    struct counter c = {.threshold = 4, .count = 0};
    if (!visit_all(values, 5, count_greater, &c) || c.count != 2) return 1;
    qsort(values, 5, sizeof values[0], order_ints);
    if (values[0] != INT_MIN || values[4] != INT_MAX) return 1;
    for (size_t i = 1; i < 5; ++i)
        if (values[i - 1] > values[i]) return 1;
    puts("callbacks: OK");
    return 0;
}
