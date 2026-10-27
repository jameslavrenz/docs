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


## A.10 A compact declaration guide

Declarations become much easier to read when they are decomposed into base type, identifier, and type constructors. The following are different contracts:

```c
int *p;                 /* pointer to an int */
int values[4];          /* four int elements */
int (*row)[4];          /* pointer to an array of four int */
int *factory(void);     /* function returning pointer to int */
int (*callback)(int);   /* pointer to function taking int */
```

The parentheses decide whether `*` applies to a named object or to the result of a function declarator. Prefer typedef names for complicated function-pointer interfaces once the meaning is understood; typedef does not create a new distinct type, but it can improve readability.

## A.11 Conversions checklist

* **Integer promotions:** narrow integer types are often promoted to `int` or `unsigned int` before arithmetic.
* **Usual arithmetic conversions:** operands in a binary arithmetic operation may be converted to a common type.
* **Assignment conversion:** the right-hand value is converted to the destination type if permitted.
* **Pointer conversions:** object pointers may convert to and from `void *` in the specified circumstances, but pointer arithmetic and access rules still apply.
* **Array-to-pointer conversion:** most array expressions convert to pointers to their first elements, with specified exceptions.

For each conversion, ask whether the mathematical value survives and whether the destination type supports the intended operation. A syntactically valid cast is not proof of a meaningful conversion.

## A.12 Alignment and lifetime at a glance

Every complete object type has an alignment requirement. Automatic storage, static storage, thread storage, and allocated storage differ in lifetime rules. A dynamically allocated region can generally hold suitably aligned ordinary objects, but special over-aligned objects may require dedicated allocation facilities or implementation-specific support.

A pointer to a released allocation is no longer a valid way to access the former object. Resetting one alias to `nullptr` does not repair other aliases. A borrowed pointer is safe only while the owner continues to supply a live object.

## A.13 C23 compatibility notes

C23's `bool`, `true`, and `false` are keywords. `nullptr` is a null pointer constant with its own type, `nullptr_t`. `static_assert`, `alignof`, and `alignas` also have modern language spellings. A new program need not imitate the macro-based spellings used historically for compatibility.

C23 has no old-style, unprototyped function declaration. Prefer `f(void)` when zero parameters are intended, even though `f()` also means zero parameters in C23. Doing so keeps intent clear when readers encounter older dialects.

The language's bit-precise integer facility, floating-point options, and standard-library additions are not uniformly available in every cross compiler. Test the exact feature on the target, record the version and mode, and provide a fallback when necessary.

## A.14 Selected undefined operations

The following are examples to recognize, not experiments whose observed results define the language:

* Out-of-bounds array accesses and invalid pointer dereference.
* Signed integer overflow and integer division by zero.
* Using an automatic object's address after its lifetime ends.
* Violating effective-type or alignment access restrictions.
* Accessing a shared non-atomic object in a data race.
* Using `realloc` with a size argument of zero in C23.
* Performing an invalid shift or violating sequencing rules.

Some implementation-defined behaviors, by contrast, are perfectly legitimate once the implementation documents them and the program accepts that platform dependency.

## A.15 References for the careful reader

The normative authority is ISO/IEC 9899:2024. For accessible navigation and implementation details, consult the C language pages at cppreference, GCC's C standards support page, and Clang's C status page. Those references help locate a topic but do not replace the exact standard text when a subtle conformance question matters.

Useful online links:

- https://en.cppreference.com/w/c/23
- https://en.cppreference.com/w/c/language
- https://gcc.gnu.org/projects/c-status.html
- https://clang.llvm.org/c_status.html
