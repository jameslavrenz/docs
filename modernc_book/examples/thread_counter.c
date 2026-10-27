/* Hosted C threads example; compile with the platform's thread flags. */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads.h>

enum { WORKERS = 2, INCREMENTS = 20000 };
struct counter_context {
    atomic_uint count;
};

static int increment(void *opaque)
{
    struct counter_context *ctx = opaque;
    for (unsigned i = 0; i < INCREMENTS; ++i)
        atomic_fetch_add_explicit(&ctx->count, 1u, memory_order_relaxed);
    return 0;
}

int main(void)
{
    struct counter_context ctx = {};
    atomic_init(&ctx.count, 0u);
    thrd_t workers[WORKERS];
    unsigned started = 0;
    bool ok = true;

    for (unsigned i = 0; i < WORKERS; ++i) {
        if (thrd_create(&workers[i], increment, &ctx) != thrd_success) {
            ok = false;
            break;
        }
        ++started;
    }

    /* Join every successfully created worker, even after a later failure.
       This ensures ctx outlives all accesses to its atomic member. */
    for (unsigned i = 0; i < started; ++i) {
        int result = -1;
        if (thrd_join(workers[i], &result) != thrd_success || result != 0)
            ok = false;
    }

    if (!ok || started != WORKERS ||
        atomic_load_explicit(&ctx.count, memory_order_relaxed) !=
            (unsigned)(WORKERS * INCREMENTS))
        return 1;
    puts("thread_counter: OK");
    return 0;
}
