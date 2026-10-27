# Appendix B. The Standard Library by Task

This is a field guide to the most commonly needed hosted C facilities, not a complete list of the standard's clauses.

| Need | Header | Key facilities |
| --- | --- | --- |
| Input, output, files | `<stdio.h>` | `FILE`, `fopen`, `fclose`, `fread`, `fwrite`, `fgets`, `printf` |
| Byte and string operations | `<string.h>` | `memcpy`, `memmove`, `memcmp`, `strlen`, `strcmp` |
| Allocation, conversion | `<stdlib.h>` | `malloc`, `calloc`, `realloc`, `free`, `strtol`, `qsort` |
| Integer limits | `<limits.h>` | `INT_MAX`, `CHAR_BIT`, etc. |
| Exact-width integer types | `<stdint.h>` | `uint32_t`, `SIZE_MAX` and related types/macros |
| Integer formatting | `<inttypes.h>` | `PRIu32`, `SCNx64` etc. |
| Type and offset | `<stddef.h>` | `size_t`, `ptrdiff_t`, `offsetof` |
| Character classification | `<ctype.h>` | `isdigit`, `isspace`, `tolower` |
| Error indications | `<errno.h>` | `errno`, `ERANGE` |
| Mathematics | `<math.h>` | `sqrt`, `sin`, `fabs`, `isfinite` |
| Assertions | `<assert.h>` | `assert` |
| Variadic functions | `<stdarg.h>` | `va_list`, `va_start`, `va_arg`, `va_end` |
| Atomics | `<stdatomic.h>` | atomic types and operations |
| Threads | `<threads.h>` | `thrd_t`, mutexes, condition variables where supported |
| Checked integer arithmetic | `<stdckdint.h>` | `ckd_add`, `ckd_sub`, `ckd_mul` (C23) |
| Bit utilities | `<stdbit.h>` | leading/trailing bit counts, population count, bit width, and bit floor/ceil (C23) |

## B.1 Length is not capacity

`strlen` finds a terminating zero byte. It does not accept a buffer size, so calling it on a nonterminated array is unsafe. Functions like `snprintf` take output capacity, and their return values should be checked to detect truncation.

```c
char buffer[16];
int needed = snprintf(buffer, sizeof buffer, "%u", 12345U);
if (needed < 0 || (size_t)needed >= sizeof buffer) {
    /* formatting error or truncation */
}
```

## B.2 Integer conversion

`strtol` is usually preferable to `atoi` when handling untrusted text. Set `errno = 0` before conversion, inspect the end pointer, and check `ERANGE`. Even after a successful `long` result, verify the destination type's range.

## B.3 Byte copy versus overlapping copy

`memcpy` requires source and destination regions not to overlap. `memmove` permits overlap, but both require valid regions of at least the requested size. Neither performs object-bound checks for you.

## B.4 Character conversion

The `<ctype.h>` functions accept `EOF` or a value representable as `unsigned char`. Passing a negative signed `char` directly is generally invalid. For byte-oriented classification, cast:

```c
unsigned char byte = (unsigned char)some_char;
if (isdigit(byte)) { /* ... */ }
```

The character functions can be locale-sensitive. They are not general Unicode text processing primitives.

## B.5 Optionality and freestanding environments

A freestanding embedded system may implement only a subset of the hosted library. Standard features may also depend on toolchain, linker, and C library combinations. Document whether examples require a hosted environment, and test all target-specific functions directly.


## B.6 A checked numeric conversion

A typical `strtol` call is more careful than merely checking whether the return value is zero:

```c
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

bool parse_positive_int(const char *text, int *out)
{
    if (text == nullptr || out == nullptr)
        return false;
    errno = 0;
    char *end;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE ||
        value < 0 || value > INT_MAX)
        return false;
    *out = (int)value;
    return true;
}
```

This accepts leading whitespace and an optional plus sign because `strtol` does. Despite its name, it also accepts zero; rename it `parse_nonnegative_int` if the name is meant to describe the precise grammar. It rejects trailing text. The output is unchanged on failure.

## B.7 Formatted-output contracts

With `printf`, each conversion specification must match the argument's promoted type. For `size_t`, use `%zu`; for a `uint32_t` whose formatting differs across platforms, use the macros supplied by `<inttypes.h>`.

A format string obtained from outside the program can be dangerous: passing it as the first argument may introduce unexpected conversion directives. To print user-controlled text, use a fixed format such as `printf("%s", input)` when a valid terminated string is guaranteed; use a counted-byte write operation for untrusted spans without terminators.

For `snprintf`, a nonnegative return value is the number of characters that would have been written, excluding the terminating zero. If that number is at least the supplied capacity, output was truncated. A negative result signals an encoding or formatting error under the interface's rules.

## B.8 Searching and sorting

`qsort` arranges objects in an array using a comparator. The comparator must interpret the correct object type and provide a consistent ordering. For integers, the result of subtracting the two values is not always representable; use relational comparisons.

`bsearch` assumes the array is sorted consistently with the supplied comparator. It does not validate this precondition. If records with equal keys have distinct associated data, the API contract must say whether any matching record is acceptable.

## B.9 Allocation and zeroed storage

`malloc` allocates uninitialized storage; `calloc` writes zero bytes to its allocated region; `realloc` may resize and move an allocation. Check all requested-size calculations before allocation. In C23, `realloc(p, 0)` is undefined and must not be used as a portable deallocation idiom.

All-bits-zero is not guaranteed to represent a null pointer or floating-point zero on every implementation. Ordinary initializer syntax such as `struct value v = {};` sets members according to the language's initialization rules, which is a different concept from a raw byte memset.

## B.10 C23 bit utilities, precisely

The new `<stdbit.h>` header supplies operations including `stdc_leading_zeros`, `stdc_trailing_zeros`, `stdc_count_ones`, `stdc_has_single_bit`, `stdc_bit_width`, `stdc_bit_floor`, and `stdc_bit_ceil`, plus related variants and byte-order macros.

It does **not** provide the C++ `std::rotl` and `std::rotr` interface. If you need rotations in C, write a properly bounded unsigned helper or use a documented implementation extension.

The associated header `<stdckdint.h>` supplies checked integer arithmetic macros. In a practical project, test whether both headers exist and link successfully with the intended C library before treating them as dependencies.

## B.11 Platform limitations

C's hosted library includes facilities that may be absent or restricted in freestanding firmware. A microcontroller environment may use retargeted `printf`, a specialized allocator, or no filesystem at all. In such a system, keep the byte-level transformation algorithms separate from device transport and I/O.

Threads and atomics also depend on implementation support. Never assume that a function which works on a desktop is safe to call inside an interrupt handler.

## B.12 Error policy examples

When a library function signals failure with a sentinel, immediately consult the documented mechanism for distinguishing failure from a normal result. For `fread`, inspect `ferror` and `feof` following a short read. For `strtol`, inspect `errno` and the end pointer. For `malloc`, check the returned pointer while remembering that zero-size requests have special semantics.

**Practice:** Create a small table containing every library function used in your program, its return-value meaning, and your application's chosen behavior on failure.
