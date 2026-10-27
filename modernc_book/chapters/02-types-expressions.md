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


## 2.11 Representable values before casts

A cast requests a conversion; it does not assert that the mathematical value fits. If an external protocol provides a `uint32_t` length and a function expects an `int` length, test the range *before* conversion. A cast performed first may already have discarded information.

```c
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

bool length_to_int(uint32_t length, int *out)
{
    if (out == nullptr || (uintmax_t)length > (uintmax_t)INT_MAX)
        return false;
    *out = (int)length;
    return true;
}
```

Here `uintmax_t` gives a common unsigned domain capable of representing both operands; the positive limit `INT_MAX` is safe to convert. The routine assigns its output only on success. This pattern works for sizes, array indexes, and serialized integer fields.

C23 requires two's-complement representation for signed integers, but that does **not** make signed overflow defined. `INT_MAX + 1` is not a portable wraparound operation. The representation model and the rules for evaluating expressions are separate questions.

## 2.12 Division, remainder, and a dangerous minimum

For integer division, the fractional part is discarded toward zero. The remainder satisfies `(a / b) * b + a % b == a` when the quotient is representable and `b` is nonzero. The expression `INT_MIN / -1` can overflow the signed result on a typical two's-complement `int` and has undefined behavior. Division by zero is undefined for integer arithmetic.

An absolute-value function with signed output cannot represent the absolute value of `INT_MIN` on the usual two's-complement implementation. Instead of writing `abs(INT_MIN)` and assuming success, choose a wider type where available or provide a checked interface.

## 2.13 Floating-point accuracy is a contract

Floating-point arithmetic has finite precision and a finite exponent range. A result can round differently from the exact real-number result; adding a very small number to a very large one may change nothing at all. Reassociating sums can alter their results:

```c
double a = 1.0e20;
double b = -1.0e20;
double c = 3.0;
/* (a+b)+c and a+(b+c) need not be equal. */
```

A numerical algorithm should state its permitted error, expected input scale, handling of infinities and NaNs, and acceptable loss of significance. The rules can also depend on whether the target implements IEC 60559 floating-point facilities. In embedded computation, fixed-point is sometimes chosen not because floating point is categorically wrong, but because reproducibility, speed, or resource bounds favor an integer representation with an explicit scale.

## 2.14 A precise conversion exercise

Write a checked function accepting a signed `long` and returning a `uint16_t` if and only if the input is between zero and `UINT16_MAX`. The correct order is: reject negative input; compare the nonnegative input in an appropriate common type; then perform the cast. Test `-1`, zero, one, the maximum, and one beyond the maximum wherever that value is representable.

Be careful with `sizeof`: its result is `size_t`, not `int`. Printing it with `%d` is an argument-type mismatch. Use `%zu` for `size_t`. Comparing object sizes and subtracting indexes also requires careful thought: an unsigned subtraction cannot express a negative distance unless the design explicitly uses a signed difference type such as `ptrdiff_t` and checks its range.

**Further problems:** Derive a safe check for `a * b` with two positive `size_t` values. Show the difference between `(unsigned)-1` and a signed negative one. Explain why shifting a negative signed value is a poor choice for portable bit-manipulation code.


## 2.15 Promotions are part of the computation

Small integer types are often promoted before arithmetic. Suppose `uint8_t a = 200, b = 100;`. On a common target where `int` represents every value of `uint8_t`, the expression `a + b` is evaluated in `int`, producing 300. Assigning that result back to `uint8_t` then performs a conversion that produces a value modulo 256. These are two different stages.

Do not infer the expression's type solely from the declared type of its operands. When shifts or bitwise complement are involved, promotions can be particularly surprising. A `~` applied to a narrow unsigned value can first promote it to `int`, yielding a value with more significant bits than the programmer expected.

If a protocol needs an eight-bit mask, perform the operation in a known unsigned domain and intentionally narrow the result only after masking:

```c
#include <stdint.h>

uint8_t low_byte(uint32_t value)
{
    return (uint8_t)(value & UINT32_C(0xff));
}
```

The mask states exactly which bits belong in the result.

## 2.16 Comparisons between signed and unsigned values

Consider `int delta = -1;` and `size_t count = 4;`. The expression `delta < count` can convert the negative `int` to an unsigned type, depending on the usual arithmetic conversions. On a typical 64-bit platform, it evaluates in a way a casual mathematical reading does not predict.

A reliable design checks negativity first:

```c
bool valid_index(int index, size_t count)
{
    return index >= 0 && (size_t)index < count;
}
```

The `&&` operation evaluates the cast and comparison only after establishing that the signed input is nonnegative. The cast is then value-preserving whenever `size_t` has sufficient range for the nonnegative `int`; implementations with unusual widths deserve a more general conversion policy.

Prefer to keep array positions as `size_t` from the beginning when negative indexes are not meaningful. But do not convert a potentially negative external value to `size_t` before validating it.

## 2.17 Division edge cases

Integer division by zero is undefined. The signed quotient may also be unrepresentable: on ordinary two's-complement systems, `INT_MIN / -1` is the familiar example. A library routine parsing a user-supplied divisor should test both conditions before evaluating the operation.

```c
#include <stdbool.h>
#include <limits.h>

bool divide_int(int a, int b, int *out)
{
    if (out == nullptr || b == 0 ||
        (a == INT_MIN && b == -1))
        return false;
    *out = a / b;
    return true;
}
```

The output is unchanged on failure. If the function returns a quotient and remainder, compute them only after the same validation. C integer division truncates toward zero; the remainder has the sign rules implied by that division, not the always-nonnegative modulo convention used in some mathematical texts.

## 2.18 Floating-point comparisons require a scale

A universal test such as `fabs(a - b) < 1e-6` is usually not a universal numeric contract. It may reject two large quantities whose relative difference is tiny, and it may accept two tiny values whose relative difference is enormous.

A numerical application should choose absolute tolerance, relative tolerance, or a combination according to the units and problem:

```c
#include <math.h>
#include <stdbool.h>

bool approximately_equal(double a, double b,
                         double absolute, double relative)
{
    if (!isfinite(a) || !isfinite(b) ||
        absolute < 0.0 || relative < 0.0)
        return false;
    double difference = fabs(a - b);
    double scale = fmax(fabs(a), fabs(b));
    return difference <= absolute || difference <= relative * scale;
}
```

This is a teaching fragment with an explicit finite-input policy; even here, intermediate overflow can make `a - b` or `relative * scale` nonfinite for extreme values. That may be acceptable for the application, or it may require a more careful algorithm. A numerical routine is correct relative to a chosen error model, not simply because it uses `double`.

## 2.19 Derive overflow checks rather than guessing

For two nonnegative `size_t` operands, `a * b` fits when either operand is zero or `a <= SIZE_MAX / b`. Notice why the division is performed only when `b != 0`. The test avoids executing multiplication until representability has been established.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool multiply_size(size_t a, size_t b, size_t *out)
{
    if (out == nullptr || (b != 0 && a > SIZE_MAX / b))
        return false;
    *out = a * b;
    return true;
}
```

When zero-sized allocations are undesirable, a caller can separately reject zero operands. Arithmetic validity and resource-policy validity are different matters.

### Worked questions

What is the type of `sizeof buffer`? Why can a byte count greater than `INT_MAX` not safely be passed as an `int` length without a range check? Why can `INT_MIN` not always be negated? What does `a & b == 0` mean without additional parentheses? For each, answer by applying the language rules rather than by running one compiler and memorizing its output.
