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


## 17.9 Make callback contracts explicit

A callback interface has two contracts rather than one. The host function promises when and how it invokes callbacks. The callback promises which values and side effects it supports. A traversal API should specify whether it visits elements in order, whether it may stop early, and whether callbacks are allowed to modify the traversed collection.

Reentrancy is another decision. If a callback invokes the traversal function again, will the operation remain correct? A traversal that uses only local state may be naturally reentrant; one that uses a static cursor may not. If callbacks can be invoked concurrently, the context object also needs an appropriate synchronization policy.

## 17.10 Context lifetime and asynchronous operations

A synchronous visitor that returns before its caller exits can safely borrow a stack-local context if it does not retain the pointer. An asynchronous driver that stores the callback and context for a later event cannot use a context that has already gone out of scope.

```c
struct counter_state { unsigned count; };

void register_callback(void (*callback)(void *), void *context);
```

This declaration alone says nothing about whether the callback is invoked immediately, on another thread, or after the registration function returns. A real API must specify that behavior. It must also provide a safe way to unregister and determine when in-flight callbacks have stopped before the context is destroyed.

## 17.11 Design a complete-write adapter

An abstract byte sink can report the number of bytes actually accepted rather than simply success or failure. A wrapper then loops until the entire record is written or the sink reports no progress or error. Such an adapter can unify a UART, a buffered memory sink, and a file stream while preserving their differing behavior at the boundary.

The wrapper must use subtraction to calculate remaining bytes, handle a zero-byte result without looping forever, and enforce the caller's storage contract. If the sink is nonblocking, `would block` may deserve a distinct status rather than being treated as fatal.

## 17.12 qsort and ordering

A comparator used with `qsort` must induce a consistent ordering. For integers, use relational comparisons rather than subtraction to avoid overflow. For structures, compare fields in a documented priority order. For floating values, decide how NaNs should be treated; ordinary relational comparisons do not automatically provide a total order over every floating value.

Sorting also changes element positions. Any pointer to an element's current location may no longer designate the same logical record after sorting. The ownership of the array itself does not change, but references to records by index may need to be updated.

## 17.13 Interfaces and performance

Function-pointer indirection can inhibit inlining and complicate static call-graph analysis, but sometimes it improves modularity enough to outweigh its cost. In firmware, an interface through function pointers may be suitable for the device layer while a hot DSP inner loop uses direct calls.

Measure on target hardware when performance matters. A desktop microbenchmark may not predict instruction cache, branch prediction, or linker optimizations on a Cortex-M core.

**Further problems:** Write a reentrant traversal that avoids global state. Define unregister semantics for an asynchronous callback API. Implement an ordered comparator for records containing a timestamp and a unique identifier.

## 17.14 A practical byte-sink interface

A callback can separate a useful algorithm from the device performing its I/O. Define a writer that receives a context pointer, source bytes, and a count, and returns how many bytes it accepted:

```c
#include <stddef.h>

typedef size_t (*sink_write_fn)(void *context,
                                const unsigned char *data,
                                size_t count);

struct byte_sink {
    sink_write_fn write;
    void *context;
};
```

The interface contract must forbid returning more bytes than requested. A zero result means no progress under this simplified synchronous protocol, so a complete-write wrapper reports failure rather than spinning indefinitely.

## 17.15 Handling partial writes

A wrapper starts at offset zero. It calls the sink with `data + offset` and `count - offset`, then increases the offset by the number of bytes accepted. Since the call occurs only when `offset < count`, these operations are within a valid caller-supplied byte buffer. If the sink returns zero or a value larger than the remaining count, the wrapper reports failure.

The loop's progress proof is straightforward: on every successful iteration, offset strictly increases and never exceeds count. The loop therefore terminates after a finite number of accepted writes, even if the sink accepts just one byte at a time.

This design is particularly relevant to a UART driver or nonblocking transport. A real asynchronous system may need a 'would block' status and a state object retained across callbacks. That is a different protocol from the simple synchronous function here.

## 17.16 Test using a fake device

The companion `examples/callback_sink.c` uses a fake writer that can accept only two bytes per call. The complete-write wrapper must repeatedly call it to deliver a longer message. The test then switches the fake into a mode that accepts zero bytes, verifying that the wrapper fails without looping forever.

A fake device is useful because it isolates the algorithm from real hardware and permits exact, repeatable failure behavior. Hardware validation remains necessary for UART timing, DMA, interrupt interaction, and other target-specific concerns.

## 17.17 Lifetime and reentrancy

The sink's context pointer is borrowed during each synchronous call. If the sink stores that pointer and uses it after returning, a new lifetime contract is required. Likewise, a sink used simultaneously by multiple threads may require a mutex or atomics. The callback pointer itself does not provide thread safety.

A reusable interface should specify whether a callback may call back into the same interface recursively. Such reentrancy can be safe for a purely local algorithm but dangerous for a driver with shared mutable state.

### Exercises

**17.6** Modify the sink to report 'would block' separately from a permanent error. **17.7** Write a wrapper that resumes a partially sent message on a later call. **17.8** Design a byte sink with a maximum record length and checked capacity. **17.9** Explain what the caller must guarantee about the lifetime of its context object.
