#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum { CAPACITY = 8 };
struct spsc {
    int items[CAPACITY];
    atomic_size_t head;
    atomic_size_t tail;
};
static size_t next(size_t pos)
{
    return pos + 1 == CAPACITY ? 0 : pos + 1;
}
static bool push(struct spsc *q, int value)
{
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t new_tail = next(tail);
    if (new_tail == atomic_load_explicit(&q->head, memory_order_acquire))
        return false;
    q->items[tail] = value;
    atomic_store_explicit(&q->tail, new_tail, memory_order_release);
    return true;
}
static bool pop(struct spsc *q, int *out)
{
    if (out == nullptr) return false;
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    if (head == atomic_load_explicit(&q->tail, memory_order_acquire))
        return false;
    *out = q->items[head];
    atomic_store_explicit(&q->head, next(head), memory_order_release);
    return true;
}
int main(void)
{
    struct spsc q = {
        .items = {0},
        .head = ATOMIC_VAR_INIT(0),
        .tail = ATOMIC_VAR_INIT(0)
    };
    int out = 99;
    if (pop(&q, &out) || out != 99) return 1;
    for (int i = 0; i < CAPACITY - 1; ++i)
        if (!push(&q, i)) return 1;
    if (push(&q, 77)) return 1;
    for (int i = 0; i < 3; ++i)
        if (!pop(&q, &out) || out != i) return 1;
    for (int i = CAPACITY - 1; i < CAPACITY + 2; ++i)
        if (!push(&q, i)) return 1;
    for (int i = 3; i < CAPACITY + 2; ++i)
        if (!pop(&q, &out) || out != i) return 1;
    if (pop(&q, &out)) return 1;
    puts("spsc_queue: OK");
    return 0;
}
