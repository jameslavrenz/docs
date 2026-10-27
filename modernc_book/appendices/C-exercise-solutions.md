# Appendix C. Selected Worked Exercises

These solutions illustrate contracts and tests rather than prescribe one clever implementation.

## C.1 Absolute difference without unsigned underflow

```c
unsigned difference(unsigned a, unsigned b)
{
    return a >= b ? a - b : b - a;
}
```

Both subtractions occur only in nonnegative order. Test `(0, 0)`, `(1, 0)`, `(0, UINT_MAX)`, and both operand orders.

## C.2 Checked parsing of an unsigned decimal

```c
#include <stdbool.h>
#include <limits.h>

bool parse_unsigned(const char *text, unsigned *out)
{
    if (text == nullptr || out == nullptr || *text == '\0')
        return false;

    unsigned value = 0;
    for (const unsigned char *p = (const unsigned char *)text;
         *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;
        unsigned digit = (unsigned)(*p - '0');
        if (value > (UINT_MAX - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    *out = value;
    return true;
}
```

The key inequality is evaluated *before* a potentially overflowing multiplication. The output is unchanged on failure. This version accepts no leading sign or whitespace, by design.

## C.3 Iterative factorial with an overflow check

```c
#include <stdbool.h>
#include <limits.h>

bool factorial(unsigned n, unsigned long *out)
{
    if (out == nullptr) return false;
    unsigned long result = 1;
    unsigned i = 2;
    while (i <= n) {
        if (result > ULONG_MAX / i) return false;
        result *= i;
        if (i == n) break;
        ++i;
    }
    *out = result;
    return true;
}
```

The explicit final-iteration break also avoids unsigned loop-counter wraparound if `n == UINT_MAX`. Checking multiplication alone is not the whole correctness proof.

## C.4 Little-endian byte decoding

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool read_u32_le(const unsigned char *bytes, size_t count, uint32_t *out)
{
    if (bytes == nullptr || out == nullptr || count < 4) return false;
    uint32_t result = (uint32_t)bytes[0]
                    | ((uint32_t)bytes[1] << 8)
                    | ((uint32_t)bytes[2] << 16)
                    | ((uint32_t)bytes[3] << 24);
    *out = result;
    return true;
}
```

The cast happens before shifting, so promotions do not make the top byte shift a signed overflow. Endianness is described by the wire format, not assumed from host memory.

## C.5 Cleanup on every path

```c
#include <stdio.h>
#include <stdbool.h>

bool write_record(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (fp == nullptr) return false;
    bool ok = fputs("record\n", fp) != EOF;
    if (fclose(fp) == EOF) ok = false;
    return ok;
}
```

Notice that `fclose` runs even after `fputs` fails. The function reports stream-level success, not guaranteed durable storage on physical media.


## C.6 A safe interval check

An interval starting at `offset` and containing `count` bytes fits within `capacity` when the start is valid and the count fits the remaining space:

```c
#include <stddef.h>
#include <stdbool.h>

bool fits(size_t offset, size_t count, size_t capacity)
{
    return offset <= capacity && count <= capacity - offset;
}
```

The short-circuit operator prevents unsigned subtraction from being evaluated if `offset` already exceeds capacity. Test zero capacity, exact fit, one-byte overrun, and `SIZE_MAX` boundaries.

## C.7 Checked allocation dimensions

```c
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

bool allocate_ints(size_t count, int **out)
{
    if (out == nullptr || count == 0 ||
        count > SIZE_MAX / sizeof(int))
        return false;
    int *p = malloc(count * sizeof *p);
    if (p == nullptr)
        return false;
    *out = p;
    return true;
}
```

The function rejects zero-sized requests and leaves the output unchanged on failure. The caller owns the returned allocation and must call `free` once. The result is not initialized; assign to elements before reading them.

## C.8 A consistent qsort comparator

```c
static int compare_ints(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}
```

Returning `a - b` could overflow for extreme signed values, invalidating the intended ordering. Use the comparator with `qsort` only when both pointers refer to valid `int` elements supplied by that call.

## C.9 A non-modifying failure test

Suppose a parser promises not to modify its output on failure. Before invoking it on an invalid string, set `unsigned result = 0x5a5aU;`. Verify the parser returns false *and* that the result remains `0x5a5aU`. Repeat with empty input, trailing junk, and an out-of-range number. A failed result alone does not establish the postcondition.

## C.10 A boundary test plan for a ring buffer

| Initial state | Operation | Required behavior |
| --- | --- | --- |
| Empty | Pop | Report false; do not change output |
| Empty | Push | Report true; count increases |
| One element | Pop | Return first item; become empty |
| Full | Push | Report false; preserve existing items |
| Partially consumed | Wraparound pushes | FIFO order is preserved |
| Returned to empty | Pop | Report false again |

Sequential tests establish logical behavior, not thread safety. The atomic SPSC queue in Chapter 16 needs additional concurrent reasoning and testing.

## C.11 How to check an exercise solution

For each solution, write down preconditions, postconditions on both success and failure, possible arithmetic overflow, and ownership effects. Then test exact boundaries. If your implementation uses a library function, read the library contract rather than assuming it matches the function's name.

Solutions are examples of reasoning, not necessarily the only acceptable design. An alternative is valid when it meets the same explicit contract.


## C.12 A counted slice and its ownership contract

A counted slice has a data pointer and a length. It does not automatically own the referenced array. The caller must keep storage alive for any operation reading the slice.

```c
#include <stddef.h>
#include <stdbool.h>

struct int_slice {
    const int *data;
    size_t length;
};

bool slice_get(struct int_slice s, size_t index, int *out)
{
    if (out == nullptr || s.data == nullptr || index >= s.length)
        return false;
    *out = s.data[index];
    return true;
}
```

On failure, the function leaves `*out` unchanged. It requires that a nonempty slice identify at least `length` accessible elements; a misleading length is a caller contract violation, not something a simple C function can discover from the pointer.

A subslice also needs to avoid unsigned overflow. Its valid-range test should be `offset <= original.length && count <= original.length - offset`, not `offset + count <= original.length`. With an empty original whose data pointer is null, return a null pointer for an empty result rather than attempting arithmetic on a null pointer.

The complete program in `examples/slices.c` exercises these rules, including `SIZE_MAX` and one-past positions.

## C.13 Failure-injected vector growth

The vector in `examples/vector_checked.c` owns its element allocation. An append operation uses a function pointer to a resizing function so a test can force reallocation to fail at a chosen point.

```c
typedef void *(*resize_fn)(void *, size_t);

static void *deny_resize(void *old, size_t bytes)
{
    (void)old;
    (void)bytes;
    return nullptr;
}
```

After filling the vector's first allocation, save the pointer, length, capacity and last element. Invoke append with `deny_resize`. A correct strong-failure implementation returns false without changing any saved property.

The second test repeats the append with a real `realloc` implementation; it then verifies the new value, the count, and safe destruction. Crucially, it does not use the old saved pointer after successful reallocation, because that pointer may be invalid even when its bit pattern looks unchanged.

This injection technique can be extended to `malloc` and to fake I/O sources. It is more reliable than hoping the operating system happens to run out of memory during a small test.

## C.14 Explain a failing test

A failed assertion is a starting point for diagnosis. Before modifying source, identify whether the program violated its own contract, the test made an invalid assumption, or the compiler did not implement the chosen language/library facility.

For example, a compiler warning that classifies a standardized C23 binary literal as a vendor extension does not automatically mean that the source is nonconforming. The response may be to use a more recent compiler or to suppress that one incorrect diagnostic while retaining other warnings. By contrast, using a macro removed in C23 is a source compatibility problem and should be corrected in the example itself.

A good verification record contains the failing command, exact diagnostic, cause, smallest corrective change, and the result of running tests on the corrected version. Avoid claiming an entire book has been verified when only a single executable has been compiled.
