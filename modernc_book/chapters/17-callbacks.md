# 17. Function Pointers, Callbacks, and Interfaces

An ordinary function call names an operation directly. A callback allows a caller to supply the operation itself. This is useful for sorting, traversing data, handling events, and separating reusable code from application policy.

## 17.1 A function pointer

Consider a function that compares two integers:

```c
static int compare_int(int a, int b)
{
    return (a > b) - (a < b);
}
```

The subtraction produces -1, 0, or 1 without calculating `a - b`, which could overflow. A pointer to such a function can be declared as:

```c
int (*comparison)(int, int) = compare_int;
int result = comparison(4, 7);
```

The parentheses around `*comparison` are essential. Without them, the declaration would describe a function returning a pointer rather than a pointer to a function.

## 17.2 A callback with context

Practical interfaces often accept both a callback and an opaque context pointer. The context lets the caller maintain state without resorting to global variables:

```c
#include <stddef.h>
#include <stdbool.h>

typedef bool (*visit_fn)(int value, void *context);

bool visit_all(const int *items, size_t count,
               visit_fn visitor, void *context)
{
    if (visitor == nullptr || (count != 0 && items == nullptr))
        return false;
    for (size_t i = 0; i < count; ++i)
        if (!visitor(items[i], context))
            return false;
    return true;
}
```

The visitor returns `true` to continue and `false` to stop. In this design `false` from the traversal means either an invalid argument or an early stop; a richer status type could distinguish them.

The `void *` context is opaque to the traversal function. The caller is responsible for supplying a pointer to the appropriate object and keeping it alive for the duration of the call. Converting an object pointer to `void *` and back preserves its intended use when the underlying type and lifetime are correct.

## 17.3 A stateful visitor

```c
struct accumulator { long long total; };

static bool accumulate(int value, void *context)
{
    struct accumulator *state = context;
    state->total += value; /* assume the sum is representable */
    return true;
}
```

A callback can change the caller's state without changing the traversal algorithm. In production, this example should either check overflow or document a bound on the count and input values.

Callbacks are usually synchronous in simple C library interfaces, but not every callback API is synchronous. An asynchronous system must state whether it retains the context pointer, which thread invokes the callback, and how cancellation and destruction interact.

## 17.4 Sorting and the comparison contract

The standard `qsort` function takes a comparator that accepts pointers to two objects and returns a negative, zero, or positive result. A comparator must be consistent; it must not produce contradictory orderings for the same values.

```c
static int order_ints(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}
```

The casts are appropriate when the comparator is called by `qsort` on an array of `int`. A comparator should not simply return `a - b` because signed subtraction may overflow. The function cannot assume that its arguments refer to arbitrary or mismatched types.

## 17.5 Function pointers are not object pointers

The language distinguishes pointers to functions from pointers to objects. Do not assume a function pointer can be stored portably in `void *`. Some operating systems provide dynamic-loading interfaces with platform-specific conventions, but those conventions are not general ISO C guarantees.

Likewise, do not manufacture a function pointer from an integer address and call it without platform-specific guarantees. Embedded startup code and vector tables sometimes rely on implementation-defined representations; such code belongs behind a documented target interface.

## 17.6 Interfaces as boundaries

A driver interface might offer methods to read, write, and close a device:

```c
#include <stddef.h>
#include <stdbool.h>

struct byte_sink {
    bool (*write)(void *context, const unsigned char *data, size_t size);
    void *context;
};
```

An application can then accept a `struct byte_sink` instead of knowing whether output goes to UART, a file, or a memory buffer. This style permits host tests using a fake sink. Yet it carries obligations: the context must remain valid, operations need failure semantics, and concurrent access must be synchronized if shared.

## 17.7 Costs and limits

Callbacks add indirection and may limit compiler inlining. That cost is often negligible compared with I/O and other operations, but it can matter in a small real-time inner loop. Measure rather than assume. A callback-based interface also needs careful error handling so that failures are not silently ignored.

## 17.8 Exercises

**17.1** Implement a visitor that counts elements greater than a threshold supplied in its context.

**17.2** Use `qsort` to order signed integers. Test `INT_MIN` and `INT_MAX` to show why a subtracting comparator is unsafe.

**17.3** Design a byte sink whose write function may accept fewer bytes than requested, and define a complete-write wrapper.

**17.4** Explain why casting a function pointer to `void *` is not a portable storage strategy.

**17.5** Show how a synchronous callback's context lifetime differs from an asynchronous callback's lifetime.
