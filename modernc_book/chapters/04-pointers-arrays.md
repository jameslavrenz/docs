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
