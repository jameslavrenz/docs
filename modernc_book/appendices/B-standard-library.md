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
| Threads | `<threads.h>` | `thrd_t`, mutexes, condition variables |
| Checked integer arithmetic | `<stdckdint.h>` | `ckd_add`, `ckd_sub`, `ckd_mul` (C23) |
| Bit utilities | `<stdbit.h>` | leading/trailing zeros, popcount, bit width, bit floor/ceil (C23) |

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


## B.13 Selecting string functions by their contracts

The C library contains several functions for operations that look similar but have importantly different preconditions.

`strlen` counts bytes in a terminated C string, excluding its final zero character. It has no separate maximum-length argument. Before calling it, the program must already know that the supplied pointer designates a valid sequence containing a terminator within accessible storage.

An arbitrary network packet or counted file buffer does not satisfy that contract merely because it contains characters. If it has no terminating zero, a call to `strlen` can read beyond the buffer. Use the supplied length for counted data.

`strcmp` compares terminated strings according to the unsigned-character values of their bytes. It is appropriate for keys stored in arrays with guaranteed termination. It is not a Unicode collation operation and does not implement locale-aware natural-language sorting.

`memcpy` copies a specified number of bytes between valid, nonoverlapping regions. `memmove` is designed for overlapping regions. Both require that the requested region fits within accessible storage. Neither checks that a caller's numerical size is truthful.

These distinctions recur throughout the book: a function name does not infer the intended storage extent, and a pointer does not carry a verified length.

## B.14 Why a bounded copy name may mislead

The traditional `strncpy` function is sometimes used as though it meant “copy a safely terminated string.” Its actual behavior is more complicated. If the source has at least as many characters as the specified count, the destination may not receive a terminating zero. If the source is shorter, the function fills the remainder of the specified region with zero bytes.

That contract can be useful for certain fixed-width fields, but it is not a general replacement for deliberate validation and copying. If the application expects a terminated string, establish the destination capacity, choose a maximum accepted length, and ensure termination explicitly.

For untrusted inputs, a clear counted-span API is often preferable. The parser can reject a length exceeding its documented maximum instead of silently truncating distinct keys into the same stored representation.

## B.15 Check formatted output carefully

The `snprintf` family permits bounded output into a supplied buffer. A typical return value represents the number of characters that would have been written, excluding the terminating zero, when no encoding error occurs. That means an apparently successful positive result can still indicate truncation.

For an ordinary nonnegative return value `n` and destination capacity `capacity`, the formatted result fits without truncation only when `(size_t)n < capacity`. The check must follow a test that `n >= 0`, so a negative error result is not converted into a huge unsigned value.

Consider:

```c
char buffer[32];
int n = snprintf(buffer, sizeof buffer, "%u", value);
if (n < 0 || (size_t)n >= sizeof buffer) {
    /* Handle encoding failure or truncation. */
}
```

This fragment assumes a valid `unsigned` value and the required `<stdio.h>` declaration. The caller can choose to reject truncation, retry with a larger buffer, or use a known fixed-size representation. What matters is that the behavior is intentional.

In a freestanding environment, the availability and completeness of formatted I/O depends on the supplied C library. Some embedded libraries intentionally omit portions of floating-point formatting to reduce code size.

## B.16 Input conversion with `strtol`

`strtol` is valuable when its grammar matches the application's requirements. It can handle a leading sign and may skip leading whitespace; it also provides an end pointer so callers can see how much input was consumed.

A robust conversion should:
1. Verify that the input is a valid terminated C string.
2. Set `errno = 0` before conversion.
3. Call `strtol` with an explicit base when appropriate.
4. Check whether the end pointer equals the start, indicating no conversion.
5. Check `errno == ERANGE` for range errors.
6. Reject unexpected trailing characters according to the application's grammar.

The parser in Chapter 2 deliberately implements a stricter grammar—optional sign followed immediately by decimal digits, with no whitespace. That is why replacing it with a bare `strtol` call would change observable behavior unless the caller added equivalent validation.

Never assume that converting a valid `long` value to a narrower destination is safe. Apply the destination's representability test separately.

## B.17 Sorting is not stable by contract

`qsort` accepts a base pointer, element count, element size, and comparator. It sorts according to that comparator but does not promise to retain the original order of elements that compare equal.

A comparator must interpret pointers as the array's actual element type and provide a consistent ordering. A comparator that subtracts signed integer keys can overflow, and one that depends on mutable global state can be inconsistent across calls.

Chapter 17's `sort_records.c` example combines priority, name, and unique arrival order. The arrival field is a final tie-breaker that produces the application's desired deterministic order without assuming the sorting algorithm is stable.

A comparator may examine pointer members inside a record only if their targets have the required lifetimes and represent valid strings or other objects throughout the sort.

## B.18 Functions involving external resources

Hosted file streams and allocated memory are external resources with lifetimes. A program that obtains them should establish where cleanup occurs, including when an intermediate operation fails.

For example, `fread` can return fewer elements than requested because of end of file or an error. Inspect `feof` and `ferror` as appropriate. `fwrite` returning fewer elements than requested indicates failure under its contract. A close can report an error after buffered output operations appeared to succeed.

A reliable program must also check counts before using the bytes as a complete record. Returning a shorter record from a read operation is not the same thing as parsing a complete valid packet.

The bounded line reader in Chapter 6 demonstrates the distinction between clean EOF, complete input, malformed input, and too-long records. Its resource model is hosted and does not imply that a microcontroller environment supplies a filesystem.

## B.19 A task-oriented checklist

Before choosing any standard-library function, identify:

- Whether the input is terminated text or counted bytes.
- Whether the source and destination storage may overlap.
- Whether the operation can allocate, block, or otherwise consume external resources.
- How it reports errors and whether any output may already have changed.
- What type conversions occur in its arguments and return value.
- Whether the target C library implements the required facility in the selected language mode.

The standard library is most useful when a program deliberately satisfies these contracts. It cannot infer a valid object lifetime, repair a dishonest buffer length, or supply a hardware feature that the deployment platform lacks.

### Reference exercise

Take one use each of `strlen`, `memcpy`, `snprintf`, `strtol`, and `qsort` in a program you have written. For each, state its actual preconditions, the relevant representation or ownership assumptions, and exactly how failure or truncation is detected. Then decide whether its use matches the intended application behavior.
