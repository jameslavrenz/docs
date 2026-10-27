/* Bounded slot pool with generation-tagged handles. No dynamic allocation. */
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

enum { SLOT_CAPACITY = 4 };

struct handle {
    size_t index;
    unsigned generation;
};

struct slot {
    bool occupied;
    bool retired;
    unsigned generation;
    int value;
};

struct pool {
    struct slot slots[SLOT_CAPACITY];
};

static bool pool_acquire(struct pool *pool, int value, struct handle *out)
{
    if (pool == nullptr || out == nullptr)
        return false;
    for (size_t i = 0; i < SLOT_CAPACITY; ++i) {
        struct slot *slot = &pool->slots[i];
        if (!slot->occupied && !slot->retired) {
            slot->value = value;
            slot->occupied = true;
            *out = (struct handle){.index = i,
                                   .generation = slot->generation};
            return true;
        }
    }
    return false;
}

/* A released handle is invalid; generation is incremented before reuse.
   Slots permanently retire at UINT_MAX to prevent generation wraparound. */
static bool pool_release(struct pool *pool, struct handle h)
{
    if (pool == nullptr || h.index >= SLOT_CAPACITY)
        return false;
    struct slot *slot = &pool->slots[h.index];
    if (!slot->occupied || slot->generation != h.generation)
        return false;
    slot->occupied = false;
    if (slot->generation == UINT_MAX)
        slot->retired = true;
    else
        ++slot->generation;
    return true;
}

/* Does not update *out on an invalid or stale handle. */
static bool pool_read(const struct pool *pool, struct handle h, int *out)
{
    if (pool == nullptr || out == nullptr || h.index >= SLOT_CAPACITY)
        return false;
    const struct slot *slot = &pool->slots[h.index];
    if (!slot->occupied || slot->generation != h.generation)
        return false;
    *out = slot->value;
    return true;
}

int main(void)
{
    struct pool p = {};
    struct handle h[SLOT_CAPACITY];
    for (int i = 0; i < SLOT_CAPACITY; ++i)
        if (!pool_acquire(&p, i * 10, &h[i]) || h[i].index != (size_t)i)
            return 1;

    struct handle untouched = {.index = 123, .generation = 456};
    if (pool_acquire(&p, 99, &untouched) ||
        untouched.index != 123 || untouched.generation != 456)
        return 1;

    int value = -7;
    if (!pool_read(&p, h[2], &value) || value != 20)
        return 1;

    if (!pool_release(&p, h[1]) || pool_release(&p, h[1]))
        return 1;

    value = -7;
    if (pool_read(&p, h[1], &value) || value != -7)
        return 1;

    struct handle replacement = {};
    if (!pool_acquire(&p, 42, &replacement) ||
        replacement.index != h[1].index ||
        replacement.generation == h[1].generation ||
        !pool_read(&p, replacement, &value) || value != 42 ||
        pool_read(&p, h[1], &value))
        return 1;

    /* Deliberately exercise generation exhaustion using a separate pool. */
    struct pool retiring = {};
    retiring.slots[0].generation = UINT_MAX;
    struct handle last = {};
    if (!pool_acquire(&retiring, 7, &last) ||
        last.index != 0 || last.generation != UINT_MAX ||
        !pool_release(&retiring, last) ||
        !retiring.slots[0].retired)
        return 1;
    struct handle next = {};
    if (!pool_acquire(&retiring, 8, &next) || next.index != 1)
        return 1;

    puts("slot_pool: OK");
    return 0;
}
