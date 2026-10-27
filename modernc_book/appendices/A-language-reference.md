# Appendix A. A Working Reference to C23

This appendix is a navigational guide, not a replacement for ISO/IEC 9899:2024. Implementations differ in optional features and in how far their C libraries have caught up.

## A.1 Translation model

A source file is preprocessed and translated as part of a *translation unit*. Header contents are incorporated by inclusion; macro substitution and conditional inclusion precede ordinary compilation. External symbols are resolved when separately compiled translation units are linked. String literals and static objects are initialized according to the rules for their storage duration.

## A.2 Fundamental types

* `void` describes absence of a value.
* `bool` is C23's boolean type. `true` and `false` are keywords.
* Character types include `char`, `signed char`, and `unsigned char`. Plain `char` is a distinct type, with implementation-defined signedness.
* Standard signed integer types are `signed char`, `short`, `int`, `long`, and `long long`; each has an unsigned counterpart.
* Floating types include `float`, `double`, and `long double`, with additional optional IEC-related facilities.
* A pointer type refers to an object or function; it does not automatically encode the size or lifetime of the referenced storage.
* Array types contain sequences of elements. Structures contain named members; unions share storage; enumerations provide named integer constants.
* C23 introduces bit-precise integer types written `_BitInt(N)` and `unsigned _BitInt(N)`, subject to implementation limits.

The sizes of fundamental types are not universally fixed. Test the actual target's limits using `<limits.h>`, `<stdint.h>`, and `<float.h>`.

## A.3 Expressions at a glance

From highest binding strength to lowest, a practical *simplified* precedence guide:

| Level | Operators or forms | Association |
| --- | --- | --- |
| Postfix | `[]` `()` `.` `->` postfix `++ --` | left to right |
| Unary | prefix `++ --`, `& * + - ! ~`, `sizeof`, `alignof` | right to left |
| Cast | `(type)` | right to left |
| Multiplicative | `* / %` | left to right |
| Additive | `+ -` | left to right |
| Shift | `<< >>` | left to right |
| Relational | `< <= > >=` | left to right |
| Equality | `== !=` | left to right |
| Bitwise AND | `&` | left to right |
| Bitwise XOR | `^` | left to right |
| Bitwise OR | `|` | left to right |
| Logical AND | `&&` | left to right |
| Logical OR | `||` | left to right |
| Conditional | `?:` | right to left (special grammar rules) |
| Assignment | `= += -= *= /= %= <<= >>= &= ^= |=` | right to left |
| Comma | `,` | left to right |

**Precedence does not imply evaluation order.** Thus `f() + g()` does not specify which call happens first. The distinction matters whenever expressions change shared state.

## A.4 Declarations

Read a declaration outward from the name, respecting parentheses:

```c
int *p;                   /* pointer to int */
int a[8];                 /* array of eight int */
int (*fp)(double);        /* pointer to function returning int */
int *make_value(void);    /* function returning pointer to int */
```

A prototype specifies parameter types. In C23, an empty function-parameter list declares a function with no parameters; `(void)` remains explicit and clear. Avoid relying on earlier C conventions for nonprototype declarations.

## A.5 Storage, linkage, and qualifiers

Automatic storage normally begins on block entry and ends when execution leaves the block. Static storage lasts for the program execution. Thread storage is associated with a thread where supported. Allocated storage lasts from successful allocation to deallocation.

At file scope, `static` can give an identifier internal linkage; `extern` can declare an externally linked identifier. `const` restricts modification through a qualified lvalue; `volatile` affects observable accesses but does not provide atomic synchronization; `restrict` is a promise about pointer-based accesses and must only be used when its aliasing conditions hold.

## A.6 Control statements

`if` chooses a branch; `switch` dispatches among case labels. `for`, `while`, and `do` iterate. `break` exits a loop or switch; `continue` advances to the next loop iteration; `return` exits a function. `goto` transfers control to a label in the same function, but must obey restrictions on entering variably modified scopes and similar rules.

## A.7 Behavior categories

* **Defined:** the standard requires specified results.
* **Implementation-defined:** the implementation chooses and documents a permitted result.
* **Unspecified:** the implementation may choose among permitted results.
* **Undefined:** no requirements are imposed; programs must not depend on an observed behavior.

A program may compile without diagnostics and still execute undefined behavior.

## A.8 Selected C23 additions

C23 standardizes binary integer literals, digit separators, empty initialization, `nullptr`, `typeof`/`typeof_unqual`, `auto` type inference, `constexpr` objects, bit-precise integers, attributes, `#embed`, `#elifdef`, and `#elifndef`. The library expands with, among other things, checked integer operations and bit utilities. Do not confuse adoption of C23 syntax in a compiler with complete deployment availability of every library feature.

## A.9 A disciplined compatibility check

Compile tiny feature probes using the real target compiler and link against the real target C library. Record compiler version, C library version, standard mode, target architecture, and the precise feature probed. Treat a feature as available only after the probe compiles, links, and where possible executes.
