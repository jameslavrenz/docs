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


## 7.15 Ownership as an interface contract

C allocation functions do not attach a runtime owner to each block. Ownership is a design convention: one component is responsible for eventually releasing a resource, while other components may temporarily borrow access to it.

A vector may own its element allocation. A slice into that vector borrows the memory. When the vector grows with `realloc`, every previously borrowed element pointer must be considered invalid, even if the new allocation happens to return the same numerical address.

A useful public header should state whether a function borrows its input, retains it after returning, or transfers ownership. For example, `vector_append` can copy an `int` value and retain no caller pointer. An asynchronous send operation that merely borrows a buffer may require the caller to keep that buffer alive until a completion callback. An API that copies the payload on submission has a different resource and timing contract.

These distinctions do not require a new language feature. They require precise documentation and tests.

## 7.16 Strong and basic failure guarantees

An append operation can promise that a failed allocation leaves the vector exactly as it was: same pointer, length, capacity, and existing elements. That is a strong failure guarantee. A more complex operation might promise only that the structure remains valid and usable, even though its contents have changed; that is a weaker but sometimes acceptable guarantee.

To achieve strong failure behavior, compute requirements locally, allocate into a temporary pointer, and commit state only after success. With `realloc`, a failed nonzero-size request leaves the old object allocated, so the implementation must not overwrite its only saved pointer with a null result.

The choice of a growth factor also affects behavior. Doubling gives amortized efficient append in the ordinary case, but may request more memory than immediately needed and can reject near-maximum capacities. A constrained embedded application may prefer fixed capacity or an arena with known bounds.

## 7.17 A checked vector implementation

The companion `examples/vector_checked.c` implements a small vector of `int`. It has initialization through an empty aggregate, append with checked capacity arithmetic, retrieval through an out-parameter, and destruction.

Its invariants are:
- `length <= capacity`;
- `data == nullptr` when capacity is zero;
- for positive capacity, `data` is an owned allocation of at least `capacity * sizeof(int)` bytes;
- elements in the half-open range `[0, length)` have been assigned meaningful values.

The append path computes a new capacity, checks both doubling and allocation-byte multiplication, and performs reallocation through a temporary pointer. If allocation fails, its earlier state remains intact. The test checks many appends, retrieval at boundaries, and destruction.

A complete library would also consider whether copying the `struct vector` itself is permitted. A shallow copy creates two apparent owners of the same allocation. For this teaching interface, copying an owning vector by assignment is prohibited by the ownership contract. C cannot enforce that restriction in the type system.

## 7.18 How to use an arena safely

An arena can provide a bounded amount of dynamic-like storage without making a separate operating-system allocation for each item. But an arena still needs an alignment policy. A request for a `double` object must be aligned suitably for `double`; a byte pointer incremented without regard to alignment is not sufficient.

When resetting the arena, think of every borrowed pointer into it as invalidated by the allocator's application-level lifetime rules. Even if old bytes remain readable on the hardware, the program must not depend on objects whose storage has been repurposed.

A simple arena is not automatically thread-safe. The same considerations apply to a pool of fixed-size objects. Concurrency, memory ordering, and synchronization remain system-level responsibilities.

## 7.19 Resource cleanup as a state machine

A function acquiring two independent resources can be modeled with states: neither acquired, buffer acquired, buffer and file acquired, and both released. Every failure transition must eventually reach the released state.

One C technique uses a single cleanup block with handles initialized to known safe values. Another uses small helper functions that release partial objects. Neither technique is inherently better in all contexts. The key is that cleanup must run on every exit path that owns resources, including paths triggered by short reads, failures to close a stream, and errors during an intermediate transformation.

Avoid placing release operations inside a boolean expression that short-circuits when an earlier operation fails. Such expressions can unintentionally skip cleanup.

## 7.20 Testing the allocator's failure paths

Ordinary desktop runs may never fail a small allocation, leaving a large fraction of an owning container's control flow untested. A test allocator can reject the next request predictably. With this seam, verify that failed append does not change length, capacity, pointer, or previously stored values.

Sanitizers can discover use-after-free and certain out-of-bounds errors along executed paths, but they do not prove the ownership design is correct. A written ownership contract plus fault-injection tests is a stronger foundation.

### Guided exercises

**7.8** Add `reserve` to the vector while preserving the strong failure guarantee. **7.9** Implement an allocator adapter that fails after N successful requests. **7.10** Explain why a shallow copy of the vector must not be destroyed independently. **7.11** Contrast an arena reset with `free` on an individual block. **7.12** Design a fixed-memory message pool for 32 records with allocation, release, and double-release detection.


## 7.21 A bounded byte arena, step by step

A byte arena offers a useful middle ground between one large static buffer and many individual heap allocations. It manages a fixed byte array and a current offset. A request for `count` bytes either returns the next available region and advances the offset or reports failure without changing state.

The complete example is `examples/arena_bytes.c`. Its essential representation is:

```c
enum { ARENA_CAPACITY = 128 };

struct byte_arena {
    unsigned char storage[ARENA_CAPACITY];
    size_t used;
};
```

The invariant is `used <= ARENA_CAPACITY`. The next available byte is `storage[used]` when `used < ARENA_CAPACITY`; `used == ARENA_CAPACITY` describes a full arena.

Imagine that the first request consumes five bytes and the second consumes seven:

```text
capacity = 128

+---------+-------------+-------------------------------------+
| 5 bytes |   7 bytes   |            free space               |
+---------+-------------+-------------------------------------+
0         5            12                                    128
                       ^
                       used
```

The next request must fit inside the remaining 116 bytes. The naive condition `used + count <= capacity` can wrap if both values are large unsigned numbers. The safe sequence is to first verify `used <= capacity`, then test `count <= capacity - used`.

This is more than a convenient idiom: it is a short proof that `storage + used` designates a position within or just beyond the array and that the requested range lies within capacity.

### A strong-failure allocation function

```c
bool arena_take(struct byte_arena *a, size_t count,
                unsigned char **out)
{
    if (a == nullptr || out == nullptr || count == 0 ||
        a->used > ARENA_CAPACITY ||
        count > ARENA_CAPACITY - a->used)
        return false;

    unsigned char *candidate = a->storage + a->used;
    a->used += count;
    *out = candidate;
    return true;
}
```

This interface explicitly rejects zero-sized requests and leaves the caller's output pointer unchanged on failure. The caller must keep the arena alive. The returned pointer is a borrowed view into arena storage; the caller must not pass it to `free`.

The arena in this example allocates *bytes*, not arbitrarily aligned typed objects. Although the array is perfectly suitable for storing octets, it does not promise all requirements for dynamically creating arbitrary C object types by casting the returned pointer. A general-purpose typed arena needs an alignment policy, suitable backing storage, and careful attention to object access and effective-type rules.

## 7.22 What reset really means

An arena reset can simply assign `used = 0`. It does not free any separate blocks because the storage belongs to the arena structure. It also does not overwrite the old bytes, which may remain physically present.

Nevertheless, the application's ownership protocol declares every previous allocation invalid after reset. A previous pointer into the array may still numerically point to an accessible byte, but that byte can now belong to a new logical allocation. Reading through the old reference may expose someone else's data or cause a logic error.

This distinction between C object lifetime and *allocator-managed logical lifetime* is important. If the arena is an automatic variable and its enclosing block ends, then the actual C object's lifetime ends as well. If only the arena offset resets, its underlying byte array still exists, but old allocations are no longer valid according to the arena API.

A robust arena user therefore treats reset as an ownership boundary. Reset only after every borrower has finished using the old allocations.

## 7.23 Alignment is a separate engineering requirement

A processor may require a `double` or 32-bit register value to begin at an address with a particular alignment. Returning the next byte without alignment adjustment may produce an address unsuitable for that type.

There are two parts to a general aligned allocation request: round the current location upward to an address satisfying the requested alignment, then verify that the padding *plus* the requested object size fits. Both arithmetic steps can overflow unless carefully checked.

A safe implementation can either support a fixed set of known alignments or require a documented power-of-two alignment no greater than the backing allocation's guarantee. On freestanding targets, DMA may impose constraints stronger than ordinary C alignment. A buffer can be correctly aligned for its type and still be unusable for an uncached DMA transfer without additional architectural measures.

The byte-arena teaching program avoids claiming those guarantees. That limitation is a feature of its contract, not a defect hidden by a cast.

## 7.24 Growable vectors versus fixed arenas

A vector owns one resizable contiguous allocation. An arena owns one fixed backing region from which many short-lived byte spans can be borrowed.

A vector is natural when one sequence grows and needs random access. An arena is useful when many temporary objects have a shared lifetime. A fixed pool of uniform records is a third choice when individual objects must be reclaimed without relocating the others.

Compare what happens on exhaustion. A vector may attempt a heap allocation and can fail if memory is unavailable or its required size overflows. An arena simply rejects a request that exceeds its fixed remaining capacity. A pool rejects a request when no free slot remains. Each has a clear failure mode, but none is automatically thread-safe.

For real-time work, do not assume that replacing `malloc` with an arena proves a deadline. You must still account for synchronization, initialization cost, memory contention, and whatever the caller does with returned storage.

## 7.25 A precise realloc failure guarantee

Suppose a vector has four stored integers and capacity four. A fifth append triggers a resize. Save the pointer, length, capacity, and existing elements before attempting the resize.

If the resizing function returns null for a nonzero request, the original allocation remains allocated. A correct implementation leaves every saved property unchanged. The test in `examples/vector_checked.c` deliberately injects such a failure through a callback, then verifies the strong failure guarantee.

On success, the allocation may be relocated. Even if the new pointer compares numerically equal to the old address, callers must derive new interior references from the returned pointer. A container interface should document that any resize invalidates previously borrowed element pointers.

Two subtle points deserve attention. First, copying the vector structure is a shallow copy, not a transfer of ownership. Second, the library should not expose a pointer into an allocation and then resize it behind the caller's back without a documented invalidation rule.

## 7.26 Building a cleanup path deliberately

A function that acquires a buffer, opens a file, and transforms data has multiple opportunities for failure. A reliable function records which resources have been acquired and ensures every acquired resource is released on all exits.

An illustrative pattern is:

```c
#include <stdio.h>
#include <stdlib.h>

int process_file(const char *path)
{
    int status = 1;
    void *buffer = nullptr;
    FILE *fp = nullptr;

    if (path == nullptr)
        return status;

    buffer = malloc(512);
    if (buffer == nullptr)
        goto cleanup;

    fp = fopen(path, "rb");
    if (fp == nullptr)
        goto cleanup;

    /* Process input; on error jump to cleanup. */
    status = 0;

cleanup:
    if (fp != nullptr && fclose(fp) == EOF)
        status = 1;
    free(buffer);
    return status;
}
```

This complete function is a *skeleton*: its processing step is omitted. The cleanup logic shows that a failed `fopen` still releases the buffer and a failed `fclose` changes the status. Calling `free(nullptr)` is permitted. A nonzero processing error would be preserved unless code later overwrote it with success, so real implementations should set success only after processing succeeds.

There is no need to avoid `goto` categorically. In C, a clearly labeled cleanup block can be easier to audit than deeply nested conditions, especially when multiple resources have independent acquisition and release steps.

## 7.27 Ownership exercise with three aliases

Consider:

```c
struct vector a = {};
/* append three elements into a */
struct vector b = a;
int *first = a.data;
```

Draw all three objects. The structures `a` and `b` are separate, but both pointers designate the same allocation; `first` also refers to that storage. If one structure calls `free` through its pointer, all aliases become unusable for accessing the former allocation. The other structure must not call `free` again.

A library can avoid such mistakes through opaque owning types, explicit move-like transfer functions, or conventions disallowing structure copying. C does not directly enforce a unique-owner type, so APIs and code review must carry the guarantee.

### Additional exercises

**7.13** Draw an arena after requests of 5, 7, and 100 bytes; identify the next legal maximum request. **7.14** Explain why `used + count <= capacity` is a weaker validation expression than `count <= capacity - used` after checking `used <= capacity`. **7.15** Add a `remaining` function that never underflows when the arena state is corrupt. **7.16** Modify an arena to support marks and rewinds; define which borrowed views are invalidated. **7.17** Introduce an injected allocation failure in a vector and prove that all prior elements survive. **7.18** Explain why a general-purpose typed arena cannot simply return any byte-aligned pointer to any requested type.
