#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct vector {
    int *data;
    size_t length;
    size_t capacity;
};

typedef void *(*resize_fn)(void *, size_t);

static void vector_destroy(struct vector *v)
{
    if (v == nullptr) return;
    free(v->data);
    *v = (struct vector){};
}

/* Contract: a failed append preserves every field and stored element. */
static bool vector_append(struct vector *v, int value, resize_fn resize)
{
    if (v == nullptr || resize == nullptr || v->length > v->capacity ||
        (v->capacity != 0 && v->data == nullptr))
        return false;

    if (v->length == v->capacity) {
        size_t capacity = 4;
        if (v->capacity != 0) {
            if (v->capacity > SIZE_MAX / 2)
                return false;
            capacity = 2 * v->capacity;
        }
        if (capacity > SIZE_MAX / sizeof *v->data)
            return false;

        void *candidate = resize(v->data, capacity * sizeof *v->data);
        if (candidate == nullptr)
            return false;

        v->data = candidate;
        v->capacity = capacity;
    }
    v->data[v->length++] = value;
    return true;
}

static bool vector_get(const struct vector *v, size_t index, int *out)
{
    if (v == nullptr || out == nullptr || v->data == nullptr ||
        index >= v->length)
        return false;
    *out = v->data[index];
    return true;
}

static void *deny_resize(void *old, size_t bytes)
{
    (void)old;
    (void)bytes;
    return nullptr;
}

int main(void)
{
    struct vector v = {};
    for (int n = 0; n < 4; ++n)
        if (!vector_append(&v, n * 3, realloc))
            return 1;

    int *old_data = v.data;
    size_t old_capacity = v.capacity;
    size_t old_length = v.length;
    int old_last = v.data[3];

    if (vector_append(&v, 50, deny_resize) ||
        v.data != old_data || v.capacity != old_capacity ||
        v.length != old_length || v.data[3] != old_last)
        return 1;

    if (!vector_append(&v, 50, realloc))
        return 1;

    int value = -7;
    if (!vector_get(&v, 4, &value) || value != 50 ||
        vector_get(&v, 5, &value) || value != 50)
        return 1;

    vector_destroy(&v);
    if (v.data != nullptr || v.capacity != 0 || v.length != 0)
        return 1;
    puts("vector_checked: OK");
    return 0;
}
