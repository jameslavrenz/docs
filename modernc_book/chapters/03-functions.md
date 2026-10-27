# 3. Functions and Program Structure

Good C programs put complexity into small, explicit interfaces. Functions make this possible, but they do not hide ownership, sizes, or error conditions unless the programmer makes those matters part of the contract.

## 3.1 Definitions, declarations, and headers

```c
/* statistics.h */
#ifndef STATISTICS_H
#define STATISTICS_H
#include <stddef.h>
double average(const double values[static 1], size_t count);
#endif
```

The `static 1` in an array parameter is a promise that an argument refers to at least one element. It is not a runtime bounds check. For an interface that accepts zero elements, prefer `const double *values` and state whether a null pointer is acceptable when `count == 0`.

```c
#include "statistics.h"

double average(const double *values, size_t count)
{
    if (count == 0) return 0.0; /* choose and document policy */
    double total = 0.0;
    for (size_t i = 0; i < count; ++i) total += values[i];
    return total / (double)count;
}
```

Zero is not always a good error signal. Later we will return status codes and write results through out-parameters.

## 3.2 Scope and lifetime

A variable defined in a block has block scope. An ordinary local variable generally has automatic storage duration: its lifetime ends when the block invocation ends. A file-scope object has static storage duration. `static` within a function preserves an object's value across calls, but introduces shared state that complicates testing and concurrent execution.

## 3.3 Recursion

```c
unsigned long factorial(unsigned n)
{
    return n <= 1 ? 1UL : (unsigned long)n * factorial(n - 1);
}
```

This mathematical definition is compact, but unsigned overflow makes its result unsuitable for large inputs, and deep recursion consumes stack. A useful recursive function requires both a terminating case and a bounded resource argument. For most production factorial calculations, an iterative implementation with an overflow check is easier to defend.

## 3.4 Design by contract

A function that converts a decimal string should say whether leading whitespace is accepted, which characters terminate conversion, how overflow is reported, and whether partial parses are accepted. A good interface often has the form

```c
bool parse_unsigned(const char *text, unsigned *result);
```

A successful call writes the result. A failed call leaves the output unchanged by convention. We can implement this without guessing whether an error sentinel might also be a valid result.

**Exercises.** 3.1 Implement `parse_unsigned` with overflow detection using `UINT_MAX`. 3.2 Replace a recursive factorial with an iterative, checked version. 3.3 Create a header/source pair and link it with a separate test file.


## 3.5 Out-parameters and explicit failure

A function that returns an integer can use every integer value as an ordinary result. That leaves no spare sentinel for failure. An interface can instead return success and write the result through a pointer:

```c
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

bool sum_nonnegative(const unsigned *items, size_t n,
                     unsigned *result)
{
    if (result == nullptr || (n != 0 && items == nullptr))
        return false;

    unsigned total = 0;
    for (size_t i = 0; i < n; ++i) {
        if (items[i] > UINT_MAX - total)
            return false;
        total += items[i];
    }
    *result = total;
    return true;
}
```

This function makes two promises: failure leaves `*result` unchanged, and success provides the exact sum. The first promise is maintained by assigning to the output only after all checks succeed. Its caller still has an obligation: the pointer must refer to a writable `unsigned` object. A null check cannot detect a dangling pointer.

The result type `bool` expresses a simple yes-or-no outcome. More elaborate interfaces can return an enumeration of errors: invalid argument, overflow, and unsupported operation are often more helpful than one generic false value.

## 3.6 Header contracts and linkage

Suppose a program has three translation units, `main.c`, `statistics.c`, and `statistics.h`. The header should state the function's public declaration and any types required to call it. The source file defining the function should include the same header; this makes disagreements visible to the compiler.

An identifier with external linkage may be referred to across translation units. A `static` function at file scope instead has internal linkage. Prefer internal linkage for helpers that are not part of the module's interface. It keeps the global symbol namespace small.

Do not put ordinary external object definitions in headers. If each including source file defines the same external object, linking can fail with multiple definitions. Place a declaration such as `extern unsigned counter;` in the header and exactly one definition `unsigned counter;` in a source file.

## 3.7 Recursion versus iteration

The mathematical definition of a recursive function does not establish that the implementation fits within the machine's resources. Each recursive invocation may need stack space. A function that explores a linked data structure may have recursion depth proportional to its length.

Use recursion when it reflects a bounded structure clearly. Prefer iteration when stack bounds are critical or when the recursive call can be replaced with an ordinary loop. For embedded programs, provide a maximum-depth argument before treating recursive code as suitable for production.

## 3.8 Exercises

**3.4** Design a `parse_temperature` interface distinguishing invalid text from out-of-range input. **3.5** Implement a checked sum for signed `int` elements. **3.6** Split `sum_nonnegative` into a public header, implementation, and executable tests. **3.7** Show how to preserve the output argument's value after each error. **3.8** Explain the difference between linkage and storage duration.


## 3.9 Choose the shape of an interface

Consider three interfaces for parsing a decimal value:

```c
unsigned parse(const char *text);
bool parse(const char *text, unsigned *out);
enum parse_status parse(const char *text, unsigned *out);
```

The first has no unambiguous error channel if every `unsigned` value is valid. The second distinguishes success from failure, and the third can explain why parsing failed. Their usefulness depends on the caller's needs. A library for configuration files might distinguish empty input, invalid character, and range error; a tiny internal helper might only require a boolean result.

An out-parameter contract should explicitly say what happens on failure. A practical convention is *strong failure safety*: validate into local state and assign to `*out` only after success. That avoids forcing callers to guess whether they can reuse a previous result following an error. It does not protect against invalid pointers supplied by callers.

## 3.10 Function declarations are contracts across files

Imagine `parse.c` defines a function but `main.c` has copied its declaration incorrectly. Separate compilation may not catch a mismatch without a shared header. Include the public header in the implementation file as well as at call sites.

```c
/* parse.h */
#ifndef PARSE_H
#define PARSE_H
#include <stdbool.h>
bool parse_unsigned(const char *text, unsigned *out);
#endif
```

The header describes the interface; it does not allocate storage or call the function. A linker can find the definition later when both translation units are linked. Be cautious about external variables in headers: `extern int errors;` is a declaration; `int errors;` can be a tentative definition and should not be duplicated indiscriminately across source files.

`static` at file scope gives helper functions internal linkage. Its purpose is not to make a function execute faster. `inline` also does not compel a compiler to substitute the body at call sites; it is part of the language's definition and linkage rules, not a measured performance guarantee.

## 3.11 A fully checked factorial

The earlier recursive factorial demonstrates recurrence, not a production overflow policy. A robust iterative version has to guard the multiplication and consider the loop-counter's boundary:

```c
#include <limits.h>
#include <stdbool.h>

bool factorial_checked(unsigned n, unsigned long *out)
{
    if (out == nullptr)
        return false;
    unsigned long value = 1;
    for (unsigned i = 2; i <= n; ) {
        if (value > ULONG_MAX / i)
            return false;
        value *= i;
        if (i == n)
            break;
        ++i;
    }
    *out = value;
    return true;
}
```

The explicit final-iteration break prevents an unsigned loop counter from wrapping when `n == UINT_MAX`. In practice the multiplication check will reject very large values long before that, but the loop's correctness should not depend on an unrelated range fact when a simple structure makes termination obvious.

## 3.12 Functions and test seams

A routine that mixes input reading, parsing, allocation, and printing is harder to test than four smaller operations with explicit contracts. The goal is not an arbitrary number of functions. It is to isolate decisions: one function transforms input bytes; one validates values; one stores them; a boundary function handles the operating environment.

For host-based firmware tests, a function that accepts a byte buffer and a length can be tested without a UART peripheral. A thin UART adapter may read bytes on the device and forward them to the parser. This separation is useful because transport errors and parsing errors have different causes and remedies.

**Further problems:** Write a header and implementation for a checked decimal parser. Define a status enumeration for invalid syntax versus overflow. Test that failure preserves the output. Explain why a static local variable may complicate recursion and concurrent calls.


## 3.13 Separate syntax checking from semantic validation

A parser does two jobs: it recognizes a sequence of characters and it interprets the recognized sequence as a value. Combining both stages into an unchecked conversion makes it easy to accidentally accept trailing garbage or overflow.

Consider a decimal parser with this contract: the input must be a nonempty sequence of ASCII digits, its value must fit in `unsigned`, and on failure the output must remain unchanged. A clear algorithm is:

1. Reject a null input or null output pointer.
2. Reject empty text.
3. Start an accumulator at zero.
4. For each byte, reject anything outside `'0'` through `'9'`.
5. Before multiplying by ten, check that the new value is representable.
6. Assign the final accumulator to the output once, after the whole string succeeds.

`examples/parse_unsigned.c` implements this design. It deliberately differs from `strtoul` by rejecting signs and spaces. Neither behavior is inherently better; the application's grammar decides.

For a parser that receives a counted span, reject both a null pointer with nonzero length and any claimed length that is not actually backed by valid storage. The latter is a caller precondition C cannot test generically.

## 3.14 Return values and out-parameters

Why return a status and write a value through an out-parameter rather than return a structure containing both? Either design can work. In C, a small result structure is often natural:

```c
struct parse_result {
    bool ok;
    unsigned value;
};
```

With this interface, the caller does not manage a separate output pointer. The function can return an all-zero result on failure, then fill both members on success. It may be clearer for small values; an out-parameter may be preferable when writing a large object or retaining a long-established ABI.

Whichever design you choose, state the failure contract. A boolean-returning function that partly overwrites the output before reporting failure is much harder for callers to use correctly than one that commits a completed result once.

## 3.15 File scope, linkage, and modules

C has no built-in class module system. The conventional unit of modularity is a header plus one or more source files. The header exposes declarations; the source file defines those operations and keeps helpers private with `static`.

A source file should include its own public header before defining the corresponding functions. That forces the compiler to compare the declared and defined types. If callers include the same header, interfaces cannot silently diverge without diagnostics.

A declaration is not always a definition. For a shared file-scope object, the header can use `extern unsigned event_count;`, while precisely one source file supplies `unsigned event_count = 0;`. Repeating a non-`extern` external object definition across headers or translation units is not a sound substitute for a module.

## 3.16 Passing arrays does not pass their sizes

Suppose a function accepts `const int values[10]`. That parameter declaration is adjusted to a pointer type; the compiler does not automatically supply a count of ten and ordinarily cannot check what the caller passed. A more honest general interface accepts `const int *values, size_t count`.

The type `const int *` says the function may not modify the elements *through that pointer*. It does not guarantee ownership, lifetime, or thread safety, and it does not make the underlying object immutable if other permitted aliases exist.

To express a fixed minimum bound, C permits array parameter syntax with `static` in the brackets. Such a declaration is a caller contract, not dynamic bounds enforcement. It should be used only when every valid call really supplies enough elements.

## 3.17 Recursive functions need a resource argument

Recursion can closely express a tree traversal or mathematical recurrence, but every call consumes some execution resources. A desktop process may have ample stack; a microcontroller task may have only a small fixed stack.

When evaluating recursive code, ask whether the maximum depth is bounded by the input contract. Can a malicious or malformed input create arbitrarily deep nesting? What happens when the recursion reaches its base case? A recursive algorithm with logarithmic depth may be reasonable; a recursive traversal of an unconstrained linked list often deserves an iterative form or a documented maximum depth.

### Guided exercises

**3.9** Implement the structure-returning parser result and compare its call sites with the out-parameter version. **3.10** Write a self-contained public header and compile it as the first include in a separate test file. **3.11** Replace a recursively implemented byte count with a loop and identify the changed resource bounds. **3.12** Design an API to read a sensor value that can report 'not ready', 'hardware error', and 'valid sample' distinctly.
