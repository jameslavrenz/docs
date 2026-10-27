/* Strict total-order comparator: never subtract potentially large values. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct record {
    unsigned priority;
    char name[16];
    size_t arrival;
};

/* All names are guaranteed NUL-terminated by this example's initializer. */
static int compare_record(const void *lhs, const void *rhs)
{
    const struct record *a = lhs;
    const struct record *b = rhs;

    if (a->priority != b->priority)
        return (a->priority > b->priority) - (a->priority < b->priority);

    int names = strcmp(a->name, b->name);
    if (names != 0)
        return (names > 0) - (names < 0);

    return (a->arrival > b->arrival) - (a->arrival < b->arrival);
}

int main(void)
{
    struct record events[] = {
        {3u, "gamma", 0},
        {0u, "zeta", 1},
        {0u, "alpha", 2},
        {3u, "alpha", 3},
        {0u, "alpha", 4},
        {3u, "gamma", 5}
    };
    const size_t count = sizeof events / sizeof events[0];

    qsort(events, count, sizeof events[0], compare_record);

    const size_t expected_arrival[] = {2, 4, 1, 3, 0, 5};
    for (size_t i = 0; i < count; ++i)
        if (events[i].arrival != expected_arrival[i])
            return 1;

    for (size_t i = 1; i < count; ++i)
        if (compare_record(&events[i - 1], &events[i]) >= 0)
            return 1;

    puts("sort_records: OK");
    return 0;
}
