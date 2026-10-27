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
| Bit utilities | `<stdbit.h>` | leading/trailing zero counts, bit width and rotation-related utilities (C23) |

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
