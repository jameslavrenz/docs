# 7. Dynamic Memory and Ownership

Every object occupies storage for a particular lifetime. Automatic objects usually live for one block invocation. Static objects live for the execution of the program. Dynamic objects, obtained through allocation functions, persist until explicitly freed.

## 7.1 Allocation is a request

```c
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

int *make_array(size_t count)
{
    if (count > SIZE_MAX / sizeof(int))
        return nullptr;
    return malloc(count * sizeof(int));
}
```

The multiplication is checked before it can overflow. A zero-sized request deserves its own interface policy: the result of `malloc(0)` is implementation-defined in useful details and should not be confused with an array containing one element. New storage from `malloc` has indeterminate contents. Use `calloc` for zero-initialized storage, while still checking the requested dimensions.

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
