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

```c compile-check
#include <stddef.h>
#include <stdbool.h>

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

```c compile-check
char name[] = "C23";
size_t capacity = sizeof name;  /* 4 including '\0' */
```

UTF-8 represents some characters with multiple bytes. A procedure that uppercases arbitrary UTF-8 by manipulating each `char` is not a correct Unicode case converter.

## 4.4 Copying bounded text

```c compile-check
#include <stddef.h>
#include <stdbool.h>

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

```c compile-check
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


## 4.15 Draw the objects before drawing pointers

For a declaration such as

```c
int values[4] = {10, 20, 30, 40};
int *p = &values[1];
```

draw four adjacent `int` objects and one separate pointer object. The pointer is not a second element of the array; it stores a value designating the second element. `p + 1` points at the third element. `*(p + 1)` reads that element. `p[1]` is defined in terms of the same indexing relationship.

Now draw `&values`. Its type is pointer to an array of four `int`, whereas `values` ordinarily converts to pointer to its first `int`. These pointers may designate the same starting address in memory, yet their types control pointer arithmetic: incrementing a pointer-to-array advances by the whole array size.

This is why declarations matter even when two pointer values print alike.

## 4.16 A counted slice

A plain pointer to the first element does not contain a length. A small value type can carry the two pieces together:

```c
#include <stddef.h>

struct int_slice {
    const int *data;
    size_t length;
};
```

The slice usually *borrows* storage. It does not allocate anything, and copying the structure copies the pointer and length rather than copying the elements. The owner must keep the array alive for the duration of every use of the slice.

A safe access function may return a boolean result and use an output pointer:

```c
#include <stdbool.h>

bool slice_get(struct int_slice s, size_t index, int *out)
{
    if (out == nullptr || index >= s.length ||
        s.data == nullptr)
        return false;
    *out = s.data[index];
    return true;
}
```

This checks the index against the *declared* slice length. Its precondition still requires `s.data` to point to at least `s.length` readable integers when `s.length` is nonzero. A dangling pointer or dishonest length cannot be repaired by the index check.

The standalone example `examples/slices.c` demonstrates both direct access and the creation of a bounded subslice.

## 4.17 Derive a subslice without overflow

A subslice starts at `offset` and contains `count` elements. It is tempting to check `offset + count <= length`, but unsigned addition may overflow. Instead, require `offset <= length` and `count <= length - offset`. That subtraction is evaluated only after the first condition holds.

When `count == 0`, you may represent the new slice using a pointer to an existing valid position, including the one-past position of a nonempty array. If the original slice is empty and its data pointer is null, do not form `data + 0`: pointer arithmetic on a null pointer is not a portable way to construct a valid empty slice. An implementation can return a null data pointer for a zero-length result.

## 4.18 Const qualifies a path of access

Compare these declarations:

```c
const int *read_only_elements;  /* elements not modifiable through pointer */
int value = 0;
int *const fixed_address = &value;
```

The first pointer variable may be reassigned, but it cannot be used to assign to the designated `int`. The second pointer variable itself cannot be reassigned, but it can modify the designated `int` if that object is modifiable and the pointer is valid.

A `const`-qualified pointer is not an ownership system. A borrowed `const char *` can become dangling when another part of the program releases its storage. Conversely, a constant object with static storage duration may remain alive for the whole program. Lifetime and permission to modify are separate properties.

## 4.19 Overlap and copying

Suppose `char letters[] = "abcdef";`. Moving the region `letters[0..3]` one position to the right creates overlapping source and destination regions. `memcpy` is not the right operation; `memmove` is specified to handle such overlap.

Before calling either library function, establish that both source and destination contain the requested number of valid bytes. The function names do not infer buffer capacity. For a type with pointer members, byte-wise copying the enclosing structure still performs a shallow copy; it does not duplicate heap storage referenced by members.

## 4.20 Pointer lifetime can end without the pointer value changing

A local variable's lifetime ends when the relevant block invocation ends. A heap allocation's lifetime ends when it is freed. Successful `realloc` ends the old allocation's lifetime and may return a different address. In each case an old pointer can retain an address-like bit pattern while becoming unusable for dereferencing the former object.

A good habit is to avoid returning borrowed pointers to transient storage. Instead return a value, let the caller supply output storage, or explicitly transfer ownership of allocated storage under an API contract.

### Guided exercises

**4.10** Draw a four-element array, a pointer to element two, and a pointer to the whole array; determine the element reached by each valid pointer increment. **4.11** Implement `slice_find` returning an index through an output pointer, distinguishing 'not found' from an invalid input. **4.12** Implement a reverse operation on a mutable slice and test empty and single-element slices. **4.13** Explain why copying a slice structure does not copy its elements. **4.14** Write a subslice function whose arithmetic remains defined at `SIZE_MAX`.


## 4.21 A worked memory diagram

The array and pointer relationship is easier to understand when we distinguish objects from the values that refer to them. Consider a five-element array and a separate pointer variable:

```c
int values[5] = {10, 20, 30, 40, 50};
int *current = &values[2];
```

One possible conceptual picture is:

```text
values:   +----+----+----+----+----+
          | 10 | 20 | 30 | 40 | 50 |
          +----+----+----+----+----+
           [0]  [1]  [2]  [3]  [4]
                      ^
current:  +-----------|-----------+
          | pointer to values[2]  |
          +-----------------------+
```

The pointer object `current` has its own storage and type. It contains a value designating `values[2]`, not a copy of the integer 30. The expression `*current = 31;` modifies the array element. The expression `current += 2;` moves the pointer to `values[4]`. One more increment produces a legitimate one-past pointer, but dereferencing that one-past pointer would be undefined.

Nothing in this picture commits C to an address size or to a particular stack layout. The array's five elements are contiguous by language definition; where the implementation stores the automatic objects is a separate matter.

## 4.22 Decoding difficult declarations

For ordinary declarations, start at the identifier and work outward using the parentheses to determine binding:

```c
int *p;                  /* pointer to int */
int a[4];                /* array of four int */
int (*row)[4];           /* pointer to array of four int */
int *function(void);     /* function returning pointer to int */
int (*callback)(int);    /* pointer to function */
```

Compare `int *function(void)` with `int (*callback)(int)`. The first declares a function, because the parentheses following `function` form the parameter list. The second declares a pointer object, because the parentheses group the `*` with `callback` before the function-parameter part is interpreted.

A `typedef` can make a complex interface more readable:

```c
typedef bool (*sample_callback)(int value, void *context);
```

It creates a name for the type, not a new distinct type with ownership or lifetime enforcement. Documentation must still tell clients how long the callback and context remain valid and whether invocation can happen asynchronously.

## 4.23 Arrays in function parameters

The declaration `void normalize(double samples[16]);` is not a request for the language to pass an entire 16-element array by value. The array parameter is adjusted to a pointer to its first element. The number 16 may document intent, but it does not enforce length at runtime.

Use a count when the function should support variable-length input:

```c
bool normalize(double *samples, size_t count);
```

This signature also forces a new question: can `samples` be null when `count == 0`? If the answer is yes, do not form an end pointer with `samples + count` before checking the null case. If the answer is no, return an error or state a stronger caller precondition.

For a fixed-size mathematical object, an array pointer can retain the row dimension:

```c
void work_on_row(int (*row)[4]);
```

The caller may pass the address of an array of four `int` values. This is not interchangeable with `int **`, which would require a pointer object at its target.

## 4.24 Building a safe subslice

A pair `(data, length)` describes an array region when the caller's preconditions are met. A subslice adds two numbers, `offset` and `count`, and must ensure the requested interval fits.

The naive condition `offset + count <= length` can overflow, because these quantities are usually `size_t`. Rewrite the condition as two comparisons:

```c
if (offset > length || count > length - offset)
    return false;
```

C's `||` operator short-circuits, so the subtraction executes only if `offset <= length`. This is an example of making a proof obligation visible in the expression itself.

The companion `examples/slices.c` demonstrates a full slice, a middle subslice, a failed request with `SIZE_MAX`, and the zero-length case. The function leaves the output view unchanged on failure, a useful promise for callers that maintain a previously valid view.

Why is a view not an owning container? Because copying it does not copy the underlying elements. It borrows the original buffer and cannot safely outlive the object that supplies those elements. When the owner reallocates or frees its storage, the view becomes invalid even if its pointer value is unchanged.

## 4.25 Multidimensional layout

A declaration `int grid[3][4];` denotes three contiguous arrays, each containing four contiguous integers. The expression `grid[r][c]` selects an element using the row and column. The address calculation is conceptually equivalent to advancing by `r * 4 + c` elements from the beginning, assuming `r < 3` and `c < 4`.

A function accepting that array can be written with a known column dimension:

```c
void inspect(size_t rows, const int grid[rows][4]);
```

The parameter is adjusted to a pointer to an array of four `const int` elements. This differs fundamentally from an array of independent row pointers, where each row may live in a different allocation.

Some APIs expose an explicit row stride or pitch because data may be padded, shared with a graphics surface, or stored by a peripheral DMA engine. A stride describes storage layout; it is not always equal to the number of logically meaningful columns. Validate both logical bounds and the physical storage extent before an indexed access.

## 4.26 String storage and counted text

A string literal includes a trailing zero character and has static storage duration. The statement `const char *name = "C23";` stores a pointer to that literal. The statement `char name[] = "C23";` defines an independently stored mutable array containing four bytes on an eight-bit-byte implementation: `'C'`, `'2'`, `'3'`, and the terminating zero.

A counted text span can represent an input containing embedded zero bytes. Such a span cannot be passed to `strlen` or `printf("%s", ...)` without additional termination guarantees. A fixed-format output like `fwrite(data, 1, length, stdout)` is appropriate for known valid byte storage, and its return value must be checked for short output.

For UTF-8, the span length counts bytes rather than Unicode scalar values or grapheme clusters. Do not assume that truncating a UTF-8 buffer to an arbitrary byte capacity preserves complete characters; a truncation may cut through a multibyte sequence.

## 4.27 Reasoning about overlap

Suppose a mutable array contains `"abcde"`, and we want to remove its second element. We can move the bytes from the third element onward left by one position, including the terminating zero if the result remains a C string.

```c
char text[] = "abcde";
memmove(&text[1], &text[2], sizeof text - 2);
```

Because the regions overlap, `memmove` is required instead of `memcpy`. The code's size calculation is valid for this fixed array, but a general function accepting `char *` must be told the meaningful length and total capacity. The destination remains within the original allocation.

When the array elements are structures that contain owning pointers, moving the record bytes is only a shallow move. The program must ensure it has not accidentally created duplicate owners or left an old record that will be destroyed twice.

### Diagnostic questions

Explain the type and validity of `&values[5]` for a five-element array. Explain why `int **` cannot receive `int grid[3][4]` directly. Describe which operation invalidates a slice into a vector after a successful resize. Show a proof that a subslice calculation cannot wrap even when `offset == SIZE_MAX`. Decide whether your interface accepts an empty slice with a null pointer.
