#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* A bounded byte arena. It makes no typed-object alignment guarantee. */
enum { ARENA_CAPACITY = 128 };

struct byte_arena {
    unsigned char storage[ARENA_CAPACITY];
    size_t used;
};

static bool arena_take(struct byte_arena *arena, size_t count,
                       unsigned char **out)
{
    if (arena == nullptr || out == nullptr || count == 0 ||
        arena->used > ARENA_CAPACITY ||
        count > ARENA_CAPACITY - arena->used)
        return false;

    unsigned char *candidate = arena->storage + arena->used;
    arena->used += count;
    *out = candidate;
    return true;
}

static void arena_reset(struct byte_arena *arena)
{
    if (arena != nullptr) arena->used = 0;
}

int main(void)
{
    struct byte_arena arena = {};
    unsigned char *first = nullptr;
    unsigned char *second = nullptr;

    if (!arena_take(&arena, 5, &first) ||
        arena.used != 5 || first != arena.storage)
        return 1;
    memcpy(first, "hello", 5);
    if (!arena_take(&arena, 7, &second) ||
        arena.used != 12 || second != arena.storage + 5)
        return 1;
    memset(second, 0xaa, 7);

    unsigned char *saved = second;
    if (arena_take(&arena, ARENA_CAPACITY, &second) ||
        arena.used != 12 || second != saved)
        return 1;
    if (arena_take(&arena, 0, &second))
        return 1;

    unsigned char *remaining = nullptr;
    if (!arena_take(&arena, ARENA_CAPACITY - 12, &remaining) ||
        arena.used != ARENA_CAPACITY)
        return 1;
    if (arena_take(&arena, 1, &remaining))
        return 1;

    arena_reset(&arena);
    if (arena.used != 0)
        return 1;
    unsigned char *whole = nullptr;
    if (!arena_take(&arena, ARENA_CAPACITY, &whole) ||
        whole != arena.storage)
        return 1;

    /* No previously borrowed pointer is used after reset. */
    puts("arena_bytes: OK");
    return 0;
}
