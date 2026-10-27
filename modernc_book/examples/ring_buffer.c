#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum { CAPACITY = 8 };
struct queue {
    unsigned data[CAPACITY];
    size_t head;
    size_t count;
};

static bool push(struct queue *q, unsigned value)
{
    if (q == nullptr || q->count == CAPACITY) return false;
    size_t tail = (q->head + q->count) % CAPACITY;
    q->data[tail] = value;
    ++q->count;
    return true;
}

static bool pop(struct queue *q, unsigned *out)
{
    if (q == nullptr || out == nullptr || q->count == 0) return false;
    *out = q->data[q->head];
    q->head = (q->head + 1) % CAPACITY;
    --q->count;
    return true;
}

int main(void)
{
    struct queue q = {};
    unsigned value = 99;
    if (pop(&q, &value) || value != 99) return 1;
    for (unsigned i = 0; i < CAPACITY; ++i)
        if (!push(&q, i)) return 1;
    if (push(&q, 999)) return 1;
    for (unsigned i = 0; i < 3; ++i)
        if (!pop(&q, &value) || value != i) return 1;
    for (unsigned i = CAPACITY; i < CAPACITY + 3; ++i)
        if (!push(&q, i)) return 1;
    for (unsigned i = 3; i < CAPACITY + 3; ++i)
        if (!pop(&q, &value) || value != i) return 1;
    if (q.count != 0 || pop(&q, &value)) return 1;
    puts("ring_buffer: OK");
    return 0;
}
