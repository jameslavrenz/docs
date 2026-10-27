# 2. Types, Expressions, and Control

The most important questions about an expression are not only what value it computes but which type it uses and whether every intermediate operation is defined.

## 2.1 Integer types and ranges

C provides `char`, `short`, `int`, `long`, and `long long`, each with signed and unsigned variants where applicable. The exact widths are implementation-dependent within standard constraints. Use `<stdint.h>` when a protocol requires an exact width such as `uint32_t`; these exact-width typedefs exist only when the implementation provides suitable types. `size_t` from `<stddef.h>` describes object sizes and array indexes.

```c
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
int main(void)
{
    uint32_t mask = UINT32_C(1) << 7;
    printf("%08" PRIx32 "\n", mask);
}
```

The formatting macros such as `PRIx32` are declared in `<inttypes.h>`, which is included above. Prefer unsigned integers for bit masks and modular arithmetic, not as a generic cure for negative values. Unsigned overflow wraps modulo a power of two; signed overflow has undefined behavior.

## 2.2 Conversion is an operation

```c
int a = 7;
int b = 2;
double x = a / b;         /* 3.0, division occurred as int */
double y = (double)a / b; /* 3.5 */
```

Conversions occur at assignments, calls, returns, and within arithmetic. Integer promotions may change a narrow type to `int`. Mixed signed and unsigned arithmetic deserves particular suspicion: compare values only after choosing an appropriate common domain.

## 2.3 Conditions and loops

The forms `if`, `else`, `switch`, `while`, `do`, and `for` govern control flow. A controlling scalar expression is false if it compares equal to zero, true otherwise. Use braces even for short branches in code that will evolve.

```c
unsigned gcd(unsigned a, unsigned b)
{
    while (b != 0) {
        unsigned remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}
```

Each iteration reduces the second operand unless it is already zero. The termination argument is as useful as the program itself: a loop should have a quantity that progresses toward a bound.

## 2.4 Short circuit and sequencing

`&&` and `||` evaluate left to right and skip the right operand when its value cannot affect the result. Thus `p != nullptr && *p == 5` can test a pointer before dereferencing it, though `nullptr` requires C23 support. Avoid compact expressions that modify the same scalar multiple times without proper sequencing: `i = i++ + 1` is not a clever increment; it has undefined behavior.

## 2.5 Floating point

Floating-point values are approximations. A decimal fraction like 0.1 often cannot be represented exactly in binary. Equality comparisons can be appropriate for exact assignments and sentinels; they are often unsuitable as tolerance tests for numerical computations. Relative and absolute tolerances should be chosen from the problem's scale, not copied as universal constants.

## 2.6 C23 spelling and initialization

C23 recognizes `bool`, `true`, and `false` as keywords, and permits empty initialization:

```c
struct point { int x, y; };
struct point origin = {};
bool ready = false;
```

An empty initializer gives the object an empty-initialization value (for this structure, both fields become zero). Check compiler support on older toolchains.

**Exercises.** 2.1 Implement absolute difference for unsigned integers without underflow. 2.2 Write `gcd` tests including zeros. 2.3 Explain why `int n = sizeof array;` can narrow an object size. 2.4 Determine which operations in a signed integer sum can overflow.


## 2.7 Integer conversion in practice

Consider `size_t`, an unsigned type used for object sizes. It is tempting to subtract two lengths and then test whether the result is negative:

```c
size_t remaining = capacity - used;
```

If `used > capacity`, this subtraction does not produce a negative number; unsigned arithmetic wraps. Instead, establish the precondition before computing the difference:

```c
if (used > capacity)
    return false;
size_t remaining = capacity - used;
```

A second trap is evaluating mixed integer types. A comparison involving an unsigned operand may convert a signed value to an unsigned type. It is better to check a negative value before a conversion than to hope the usual arithmetic conversions preserve its mathematical meaning.

## 2.8 Bitwise operations

Bit masks encode independent boolean properties in one integer. Shift counts must be valid for the promoted left operand's width. For portable protocol data, perform shifts in an explicitly selected unsigned type.

```c
#include <stdint.h>

enum {
    FRAME_READY = 1u << 0,
    FRAME_ERROR = 1u << 1,
    FRAME_RETRY = 1u << 2
};

unsigned flags = FRAME_READY | FRAME_RETRY;
bool retry = (flags & FRAME_RETRY) != 0;
flags &= ~FRAME_RETRY;
```

The `|` operator combines selected bits, `&` tests or retains bits, and `~` complements all bits in the promoted type. Using a mask is not the same as testing a boolean expression with `||` or `&&`; those logical operators produce zero or one and may short-circuit.

An important embedded distinction: masking an integer held in RAM is straightforward; read-modify-write of a device register may trigger hardware-specific behavior. Consult the peripheral's reference manual before applying an apparently innocent `register |= bit`.

## 2.9 Evaluation order is independent of precedence

Multiplication binds more tightly than addition, but precedence tells the compiler how expressions are grouped, not the order of their operands' evaluation. For example, `f() + g()` does not impose an ordering between calls. The programmer can create a sequence explicitly:

```c
int left = f();
int right = g();
int result = left + right;
```

Now the calls happen in the order of the statements, though addition still requires a representable result. Avoid constructions that both modify and read a scalar without the necessary sequencing. Compact code is not an excuse for surprising evaluation.

## 2.10 A checked average

For integers in a constrained environment, accumulating into a wider type can help, but a wider type is not a proof of no overflow. Even a 64-bit sum can overflow if the input is long enough. A fully safe routine must either check each addition, constrain the input count and element ranges, or use a documented alternative algorithm.

A good interface for an average also distinguishes the empty-input case from a real average of zero. A return value alone does not always provide enough information.

### Further exercises

**2.5** Write a function that sets and clears one bit of a `uint32_t` given a bit position. Reject positions outside 0–31. **2.6** Show an example in which adding two valid `int` values overflows, and implement a guard that avoids evaluating the addition. **2.7** Explain the difference between `a & b == 0` and `(a & b) == 0`. **2.8** Design a loop over `unsigned` values that handles `UINT_MAX` without wrapping forever.
