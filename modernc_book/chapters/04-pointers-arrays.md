# 4. Pointers, Arrays, and Strings

Arrays own contiguous elements; pointers refer to objects or functions. The relationship is close, but the two are not the same thing.

## 4.1 Address and indirection

```c
int value = 12;
int *p = &value;
*p = 13;          /* modifies value */
```

A pointer's validity depends on the referenced object's lifetime, alignment, and access permissions. Dereferencing a null, dangling, or otherwise invalid pointer has undefined behavior. C23 adds the keyword `nullptr` for a null pointer constant; legacy code commonly uses `NULL`.

## 4.2 Array arguments carry no length

```c
#include <stddef.h>

int sum(const int *data, size_t count, long long *out)
{
    if (out == nullptr || (count != 0 && data == nullptr))
        return -1;
    long long total = 0;
    for (size_t i = 0; i < count; ++i)
        total += data[i]; /* caller must ensure representable result */
    *out = total;
    return 0;
}
```

The signature carries a count because a pointer does not record the bounds of the original array. This implementation still assumes the sum fits in `long long`; a fully checked version needs an overflow test before each addition.

## 4.3 Strings are terminated byte sequences

A C string is an array of `char` with a terminating zero byte. `strlen` measures the number of bytes before that terminator, not necessarily the number of Unicode characters. `sizeof` measures a type or an object, not the current length of a string.

```c
char name[] = "C23";
size_t capacity = sizeof name;  /* 4 including '\0' */
```

UTF-8 represents some characters with multiple bytes. A procedure that uppercases arbitrary UTF-8 by manipulating each `char` is not a correct Unicode case converter.

## 4.4 Copying bounded text

```c
#include <stddef.h>

bool copy_text(char *dst, size_t capacity, const char *src)
{
    if (dst == nullptr || src == nullptr || capacity == 0)
        return false;
    size_t i = 0;
    while (i + 1 < capacity && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
    return src[i] == '\0';
}
```

This function assumes valid source storage and that source and destination do not overlap; it signals truncation rather than silently claiming success. It is not a replacement for a general `memmove`.

## 4.5 Constness and pointer arithmetic

`const int *p` prohibits modifying an `int` through `p`. `int *const p` prohibits assigning another address to `p`. Pointer arithmetic is defined only within an array object (including the permitted one-past position), subject to the language's constraints. A one-past pointer may be compared or subtracted appropriately but not dereferenced.

**Exercises.** 4.1 Implement `strlen` using a pointer and compare with an index version. 4.2 Explain the difference between `char *p = "abc";` and `char p[] = "abc";`. 4.3 Implement `reverse` for a mutable byte array of known length. 4.4 Add overlap detection or explicitly reject overlap in a copy API.


## 4.6 A pointer is not an array descriptor

The expression `sizeof array` is useful inside the scope where `array` is an actual array object. Once an array expression is converted to a pointer for passing to a function, its bound is ordinarily lost.

```c
#include <stddef.h>

static size_t count_values(const int *values, size_t count)
{
    size_t found = 0;
    for (size_t i = 0; i < count; ++i)
        if (values[i] != 0) ++found;
    return found;
}

int main(void)
{
    int values[] = {3, 0, 8, 0, 1};
    size_t count = sizeof values / sizeof values[0];
    return count_values(values, count) == 3 ? 0 : 1;
}
```

The caller supplies the element count. The callee accepts that count as a contract; C cannot generally establish that a non-null pointer references as many elements as claimed.

## 4.7 Two-dimensional arrays

A real two-dimensional array occupies contiguous storage in row-major order:

```c
int matrix[2][3] = {
    {1, 2, 3},
    {4, 5, 6}
};
```

The expression `matrix` in most contexts converts to a pointer to an array of three `int`, not an `int **`. This distinction is essential: a pointer-to-pointer usually points to another pointer object, whereas a 2-D array's rows are laid out directly next to each other.

A function that requires a known column count can state it:

```c
int matrix_total(size_t rows, const int values[rows][3])
{
    int result = 0; /* contract: sum must fit in int */
    for (size_t r = 0; r < rows; ++r)
        for (size_t c = 0; c < 3; ++c)
            result += values[r][c];
    return result;
}
```

C23 retains variable length array support as specified by the standard, with implementation differences relevant to storage-duration choices. Where fixed dimensions suffice, fixed arrays make capacity easier to reason about.

## 4.8 Lifetime is as important as address

Never return the address of an automatic local object:

```c
int *bad_pointer(void)
{
    int local = 7;
    return &local; /* invalid after this invocation ends */
}
```

The problem is not that the stack address must immediately change; it is that the object's lifetime has ended. The behavior of later dereferencing that pointer is not defined. Returning a value, writing through a caller-supplied pointer, or returning an allocated object with a documented owner are legitimate alternatives.

## 4.9 Restrict and aliasing

The qualifier `restrict` describes an access pattern that permits optimizations when the programmer satisfies its association requirements. It is a promise, not a runtime test. Do not add it to a public API merely because it sounds faster. If two pointer arguments can legally refer to overlapping storage, the interface must not assert a restriction that the caller violates.

## 4.10 Exercises

**4.5** Implement an in-place reversal of a `char` array given its exact length. **4.6** Explain why `int **p = matrix;` is incompatible with a two-dimensional `int matrix[2][3]`. **4.7** Write a function to find a value in a slice represented by pointer and length. **4.8** Return both minimum and maximum through checked out-parameters. **4.9** Explain what happens to pointers into a block after successful `realloc`.


## 4.11 Three meanings of an array name

In many expressions an array expression converts to a pointer to its first element. Important exceptions include operands of `sizeof`, unary `&`, and other contexts with specified non-decay behavior. For a declared array:

```c
int items[5] = {};
size_t bytes = sizeof items;
int *element = items;
int (*whole)[5] = &items;
```

`element` points to an `int`; `whole` points to the entire five-element array. Incrementing the former advances by one `int`; incrementing the latter would advance by one whole array. The types convey this difference and permit the compiler to diagnose incompatible pointer uses.

When an array is passed as a function argument, a declaration such as `void process(int values[10]);` still adjusts to a pointer parameter. The `10` in that form does not create a checked ten-element slice. Carry a count unless the function's contract fixes the dimension and callers are required to honor it.

## 4.12 One past the end is not an element

Pointer arithmetic can produce the one-past position of an array. It is useful as an end marker, but it cannot be dereferenced. For a nonempty array, `begin = &items[0]` and `end = begin + count` can represent a half-open range `[begin, end)`.

Comparing or subtracting arbitrary unrelated object pointers is not the same as comparing array positions. Pointer subtraction is defined for pointers into the same array (including one past), subject to representability in `ptrdiff_t`. When `count` may be enormous, indexes of type `size_t` can be more straightforward than pointer differences.

A null pointer is not an empty array that can be incremented freely. A zero-length buffer interface should explicitly describe whether it accepts `nullptr`, and code must avoid forming `nullptr + 0` merely to make an end pointer.

## 4.13 Copy, move, and overlap

`memcpy` copies object-representation bytes from one valid region to another but requires nonoverlapping regions. `memmove` supports overlap. Both require sufficiently large regions. Neither can prove that a claimed size is correct.

A safe shift within an array might use:

```c
#include <string.h>
/* Preconditions: capacity >= count + 1 and position <= count. */
memmove(&items[position + 1], &items[position],
        (count - position) * sizeof items[0]);
```

Those preconditions matter. Check capacity and `position` before computing an address or multiplication. Also ensure the item type permits moving its object representation in the intended way; plain C value records generally do, whereas pointers inside records may carry ownership semantics not preserved by shallow copying.

## 4.14 String ownership and views

A string literal has static storage duration; modifying it through a pointer produces undefined behavior. By contrast, `char text[] = "abc";` defines a mutable array whose elements can be changed. The type `const char *` commonly signals that a function borrows text without modifying it, but `const` alone does not guarantee the memory remains alive.

A counted string view, `struct text_view { const char *data; size_t size; };`, can represent slices that are not null-terminated. Such views are convenient for parsers, but passing their pointers to `strlen` or `printf("%s")` is unsafe unless termination is separately guaranteed. A view usually borrows storage and should not outlive its owner.

**Further problems:** Implement a bounded substring search that never relies on a terminator. Explain the difference between `int (*)[4]` and `int **`. Write tests for a zero-length slice and for the final legal element index.
