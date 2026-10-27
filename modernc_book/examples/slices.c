#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct int_slice {
    const int *data;
    size_t length;
};

static bool slice_get(struct int_slice slice, size_t index, int *out)
{
    if (out == nullptr || slice.data == nullptr ||
        index >= slice.length)
        return false;
    *out = slice.data[index];
    return true;
}

static bool slice_subslice(struct int_slice input, size_t offset,
                           size_t count, struct int_slice *out)
{
    if (out == nullptr || (input.length != 0 && input.data == nullptr) ||
        offset > input.length || count > input.length - offset)
        return false;

    struct int_slice result = {
        .data = input.data == nullptr ? nullptr : input.data + offset,
        .length = count
    };
    *out = result;
    return true;
}

int main(void)
{
    const int values[] = {10, 20, 30, 40, 50};
    struct int_slice full = {values, 5};
    struct int_slice sub = {};
    int value = -9;

    if (!slice_subslice(full, 1, 3, &sub) ||
        sub.length != 3 || !slice_get(sub, 0, &value) ||
        value != 20 || !slice_get(sub, 2, &value) || value != 40)
        return 1;

    if (slice_get(sub, 3, &value) || value != 40)
        return 1;

    const struct int_slice saved = sub;
    if (slice_subslice(full, SIZE_MAX, 1, &sub) ||
        sub.data != saved.data || sub.length != saved.length)
        return 1;

    if (!slice_subslice((struct int_slice){nullptr, 0}, 0, 0, &sub) ||
        sub.data != nullptr || sub.length != 0)
        return 1;

    if (!slice_subslice(full, 5, 0, &sub) ||
        sub.length != 0 || sub.data != values + 5)
        return 1;

    puts("slices: OK");
    return 0;
}
