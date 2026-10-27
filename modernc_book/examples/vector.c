#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct vector { int *data; size_t length, capacity; };

static void vector_destroy(struct vector *v) {
    if (v == nullptr) return;
    free(v->data);
    *v = (struct vector){};
}
static int vector_append(struct vector *v, int value) {
    if (v == nullptr) return 0;
    if (v->length == v->capacity) {
        size_t next;
        if (v->capacity == 0) next = 4;
        else {
            if (v->capacity > SIZE_MAX / 2) return 0;
            next = v->capacity * 2;
        }
        if (next > SIZE_MAX / sizeof *v->data) return 0;
        void *memory = realloc(v->data, next * sizeof *v->data);
        if (memory == nullptr) return 0;
        v->data = memory;
        v->capacity = next;
    }
    v->data[v->length++] = value;
    return 1;
}
int main(void) {
    struct vector v = {};
    for (int i = 0; i != 100; ++i)
        if (!vector_append(&v, i * 2)) { vector_destroy(&v); return 1; }
    if (v.length != 100 || v.data[99] != 198) {
        vector_destroy(&v);
        return 1;
    }
    vector_destroy(&v);
    if (v.data != nullptr || v.length != 0 || v.capacity != 0) return 1;
    puts("vector: OK");
    return 0;
}
