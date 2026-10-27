# 7. Dynamic Memory and Ownership

Every object occupies storage for a particular lifetime. Automatic objects usually live for one block invocation. Static objects live for the execution of the program. Dynamic objects, obtained through allocation functions, persist until explicitly freed.

## 7.1 Allocation is a request

```c
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

int *make_array(size_t count)
{
    if (count == 0 || count > SIZE_MAX / sizeof(int))
        return nullptr;
    return malloc(count * sizeof(int));
}
```

The multiplication is checked before it can overflow. This interface rejects zero-sized requests. Other interfaces may choose different zero-size semantics, but they should document them. New storage from `malloc` has indeterminate contents. Use `calloc` for zero-initialized storage, while still checking the requested dimensions.

## 7.2 One owner, one release

```c
int *samples = make_array(100);
if (samples == nullptr)
    return 1;
/* use samples[0] through samples[99] */
free(samples);
samples = nullptr;
```

The variable `samples` is not the allocated object; it holds an address. After `free`, all aliases that would access the old object are invalid. Resetting one variable does not repair copies of the address.

A useful API states: who allocates, who frees, whether buffers may be shared, which functions can retain pointers, and which operations invalidate earlier pointers. These are conventions implemented by programmers, not runtime guarantees supplied by the language.

## 7.3 Resize without losing the original

```c
void *tmp = realloc(buffer, new_size);
if (tmp == nullptr) {
    /* buffer remains allocated when new_size > 0 */
    return false;
}
buffer = tmp;
```

The example assumes `new_size > 0` and that `buffer` is a valid reallocatable allocation. Assigning `realloc` directly to the only saved pointer risks a leak on failure. Successful reallocation may move storage, invalidating interior pointers.

## 7.4 Fixed-capacity alternatives

Embedded systems often trade flexible allocation for predictable resource bounds. A fixed-size arena can allocate successive objects from a caller-provided array. It must honor alignment and report exhaustion; an arena reset invalidates all objects in it. Pools that recycle uniform slots provide a different tradeoff. Neither allocator model is automatically thread-safe.

## 7.5 A resource table

| Resource | Acquisition | Release |
| --- | --- | --- |
| Heap block | `malloc`, `calloc` | `free` |
| File stream | `fopen` | `fclose` |
| Automatic array | enter block | leave block |
| Static object | program initialization | program termination |

**Exercises.** 7.1 Implement checked dynamic-array growth. 7.2 Write a fixed-capacity arena with explicit alignment policy. 7.3 Identify double-free, leak, and use-after-free paths in a linked-list teardown.


## 7.6 A checked growable vector

A dynamic array needs three measurements: the pointer to the allocation, the number of initialized elements, and its capacity. The invariant is `length <= capacity`. Growing is exceptional, not required on every insertion.

```c
struct vector {
    int *data;
    size_t length;
    size_t capacity;
};
```

A growth policy might double capacity, but the multiplication needs an overflow check. When capacity cannot double, the allocator must either use a smaller sufficient capacity or report failure. The success of `realloc` invalidates pointers into the old allocation even if the returned address happens to equal the old address; callers should obtain fresh pointers from the vector after a change.

The vector's *ownership* can be stated simply: it owns `data`, its caller owns the vector structure, and exactly one destroy operation releases the allocation. Copying the structure by assignment copies its pointer, not its ownership. Two such copies must not both be destroyed.

## 7.7 An arena with an explicit alignment policy

An arena hands out storage from a fixed block. Its main benefits are bounded overall consumption and cheap bulk reset. Correct alignment must be established for each allocation. A simple arena supporting only objects with alignment no stricter than `max_align_t` can use storage suitably aligned at its base, round offsets upward without overflow, and reject requests that do not fit.

Alignment is part of the contract, not a detail to ignore because a particular CPU tolerates misaligned loads. DMA buffers and cache maintenance impose further target-specific constraints that a generic C arena does not automatically meet.

## 7.8 Failure-safe initialization

Consider creating an object that owns a buffer and opens a file. If the second step fails, the first resource must be released. Design initialization so that either every required resource is acquired, or every partial acquisition is undone. A cleanup label can be a sensible C technique when a function has many resource-acquisition steps.

```c
int status = 1;
void *buffer = malloc(1024);
if (buffer == nullptr) goto done;
FILE *file = fopen("data.bin", "rb");
if (file == nullptr) goto done;
/* use resources */
if (fclose(file) != 0) goto done;
status = 0;
done:
free(buffer);
return status;
```

This sketch requires `<stdio.h>` and `<stdlib.h>`. It closes the file on the successful path, but it is not a general model for cleanup after all possible failures within the use block. For several resources, initialize handles to known values and centralize cleanup that checks each handle.

## 7.9 Exercises

**7.4** Implement a vector with `init`, `append`, and `destroy`. **7.5** Demonstrate how copying a vector structure creates two pointer aliases. **7.6** Design a resettable arena and describe which pointers become invalid after reset. **7.7** Write a function that acquires two resources and releases both on every failure path.


## 7.10 Distinguish capacity from initialized length

An allocation of space for ten integers does not automatically initialize ten integer values. With `malloc`, reading elements before they are assigned is invalid. With `calloc`, the allocated bytes are zero-initialized; this yields the all-bits-zero representation of every byte, which for integer objects gives the value zero. For pointers and some other types, all-bits-zero bytes are not universally a promise of the semantic null or zero value on every conceivable C implementation.

A vector records both allocated *capacity* and initialized *length*. An append operation writes an element into unused capacity before increasing the length. A destroy operation releases the allocation once and invalidates all borrowed element pointers.

When designing a C API, use distinct names for these concepts. Confusing `length` and `capacity` is a recurring source of out-of-bounds reads and writes.

## 7.11 Zero-size requests in C23

A historical compatibility trap deserves explicit attention. In C23, calling `realloc(ptr, 0)` has undefined behavior. Avoid using it as a shorthand for freeing a block. Instead, use `free(ptr)` when the desired operation is release, and define an explicit empty-state policy in your container API.

The behavior of `malloc(0)` is not a portable way to obtain a usable object. An API can reject zero-sized element arrays, represent them with a null pointer and zero capacity, or use another documented convention. The key is consistency at every call site.

## 7.12 Checked growth without losing storage

To grow an array, first choose a new capacity greater than or equal to the required length; then check whether `capacity * sizeof element` fits in `size_t`; then call `realloc` using a temporary pointer. Only on success do you update the stored pointer and capacity.

The example vector in `examples/vector.c` follows this approach for simple `int` elements. Because it uses a doubling policy, a very large capacity may be rejected rather than trying every possible smaller capacity. That is an acceptable documented policy for a teaching example; it is not the only valid design.

No pointer into an allocation may be assumed valid after a successful `realloc`. Recompute interior pointers from the returned base. A failed nonzero-size `realloc` leaves the old allocation intact, which is why assigning the result directly to the only owner is dangerous.

## 7.13 Alignment and arenas

C object types have alignment requirements. A byte arena can hand out storage for types only when it aligns the returned address appropriately and has enough remaining bytes. Rounding an offset upward can overflow; check the arithmetic. A generic aligned allocator must consider the actual address of the backing allocation, not just an offset from an arbitrarily aligned base.

If an arena is reset, every allocation it had issued becomes invalid for application purposes, even though the bytes physically remain. The arena is attractive when a whole group of temporary objects has the same lifetime. A free-list pool is better when individual fixed-size objects must be reclaimed independently.

## 7.14 Failure injection as a learning tool

Ordinary runs rarely exercise allocation failures. For a unit test, wrap `malloc` and `realloc` behind function pointers or an allocator object, then provide a test implementation that fails on a specified request. Confirm that failure neither leaks the old allocation nor changes the vector's logical length or existing element values.

This is a useful general lesson: error paths deserve executable tests, not just reassuring comments.

**Further problems:** Modify the vector to distinguish allocation failure from arithmetic overflow. Implement `vector_reserve` that leaves all state untouched on failure. State the ownership implications of `struct vector copy = original;`. Write an arena allocation test covering zero bytes, alignment and exhaustion.
