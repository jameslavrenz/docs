# 19. Compile-Time Programming and the Preprocessor

The C preprocessor is a separate, token-oriented facility that runs before ordinary translation. It can select platform-specific code, define constants, construct repetitive expressions, and include headers. Its power makes restraint important: a macro is not a typed function, and conditional compilation should not obscure the program's logic.

## 19.1 Include files and translation units

A header normally contains declarations and interface-related definitions. A source file includes that header and supplies the corresponding external definitions. Inclusion is conceptually textual: the preprocessor inserts the header's tokens into the translation unit.

```c
#ifndef BUFFER_H
#define BUFFER_H
#include <stddef.h>
struct buffer_view { const unsigned char *data; size_t size; };
#endif
```

The guard prevents repeated processing of the same interface within one translation unit. It does not make two incompatible versions of an interface agree or prevent multiple external definitions across translation units.

## 19.2 Object-like macros

An object-like macro can name a manifest constant:

```c
#define BUFFER_CAPACITY 256
```

For a numeric quantity with an appropriate integer type, an enumeration constant or C23 `constexpr` object can be clearer, subject to the required constant-expression context and toolchain support. A macro has no type of its own; it substitutes preprocessing tokens that are interpreted later.

## 19.3 Function-like macros and double evaluation

```c
#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))
```

This macro can evaluate a chosen argument twice. An invocation involving `i++` is dangerous even if its parentheses are correct. Replacing a macro with a `static inline` function usually improves type checking and evaluation behavior, but an inline function still has to handle overflow and other ordinary C hazards.

## 19.4 Stringizing and token pasting

The `#` operator can stringify a macro argument, and `##` can paste preprocessing tokens. These mechanisms are helpful for diagnostics and repetitive declarations but can create opaque interfaces.

```c
#define STRINGIZE(x) #x
#define CONCAT(a, b) a ## b
```

For example, `STRINGIZE(hello)` produces the string literal `"hello"`. Token-pasting does not create arbitrary source text; its result must be a valid preprocessing token. Expand-once helper macros may be necessary when callers pass macro arguments whose expanded values should be concatenated.

## 19.5 Conditional compilation

```c
#if defined(PLATFORM_DEVICE)
    /* device adaptation */
#elif defined(PLATFORM_HOST)
    /* host adaptation */
#else
    #error "Choose a supported platform"
#endif
```

C23 also provides `#elifdef` and `#elifndef`. Use feature probes where possible: checking a vendor name is not proof that a particular header or library function is available. Keep platform branches behind narrow adapter interfaces so most application code can be compiled identically everywhere.

## 19.6 C23 embedding and diagnostics

The C23 `#embed` directive is designed to incorporate resource data into a translation unit. It is particularly interesting for fixed firmware resources such as calibration tables or static model parameters. Actual support differs among compilers, so a production project needs a feature probe and a fallback generation step.

C23 also standardizes `#warning`. Diagnostics emitted this way can be useful when a configuration is supported but discouraged. Unlike `#error`, a warning does not necessarily stop translation.

## 19.7 Static assertions

Use `static_assert` to make fundamental assumptions visible at translation time:

```c
#include <limits.h>
static_assert(CHAR_BIT == 8, "octet storage required");
```

The constraint is appropriate for an octet-specific wire format; it would be unnecessarily restrictive for a general-purpose algorithm that does not depend on byte width.

## 19.8 Generic selection

The `_Generic` selection mechanism works on types rather than token spelling. It can choose an appropriate typed operation:

```c
#include <math.h>
#define abs_floating(x) _Generic((x), \
    float: fabsf, double: fabs, long double: fabsl)(x)
```

The selected function takes the value once, avoiding a common multiple-evaluation problem of expression macros. This example requires a supported floating type. A more complete public API should decide how to diagnose unsupported types and document conversion policy.

## 19.9 An example of compile-time checks

The companion program `examples/compile_time.c` combines `static_assert`, stringizing, token pasting, a C23 empty initializer, and a small function. Its checks are deliberately modest: their purpose is to show that preprocessing and language-level type checking cooperate but are not identical mechanisms.

## 19.10 Exercises

**19.1** Explain why a function-like macro can evaluate an argument more than once.

**19.2** Implement two-level stringizing that expands a macro before turning its value into a string.

**19.3** Write a configuration header that rejects simultaneous host and device target selections.

**19.4** Write a compile probe for `#embed` and document a fallback for a toolchain that lacks it.

**19.5** Define the differences among `#if`, `static_assert`, and a runtime `if` statement.

**19.6** Use a `_Generic` selection to choose one of two distinct, typed integer-printing functions.


## 19.11 Preprocessing is not type checking

The preprocessor expands tokens before the compiler evaluates ordinary C types and expressions. This means a macro can construct an expression that looks plausible but has surprising side effects. A typed function receives evaluated arguments once for each parameter, but the order of evaluation among arguments is still not specified merely by the call.

Prefer `enum` constants, appropriately typed `constexpr` objects where supported, and `static inline` functions for tasks that do not require token substitution. Reserve macros for conditional compilation, stringizing, token-pasting, or genuinely generic constructs that cannot be expressed as ordinary C functions.

## 19.12 Expand macros deliberately

Two-level macros are needed when a macro argument should be expanded before being stringized:

```c
#define STR_DIRECT(x) #x
#define STR_EXPANDED(x) STR_DIRECT(x)
#define MAJOR_VERSION 3

const char *version = STR_EXPANDED(MAJOR_VERSION);
```

`STR_DIRECT(MAJOR_VERSION)` yields the identifier spelling, while `STR_EXPANDED(MAJOR_VERSION)` yields `"3"`. Macro expansion rules are precise but easy to misread. Prefer explicit, small helper macros with a test illustrating their purpose.

## 19.13 Conditional compilation and dead branches

A `#if` directive removes or retains preprocessing tokens before the compiler type-checks ordinary statements. A runtime `if` statement, even if its condition is a constant expression, still belongs to the translated C program and is subject to language constraints.

Use conditional compilation for platform-specific includes, unsupported instructions, or truly unavailable APIs. When both implementations are valid C on the current target, a normal function call or runtime branch may be easier to maintain.

## 19.14 Using embed responsibly

C23's `#embed` is valuable for incorporating binary resources into source initialization. A resource might contain firmware calibration constants or a model binary. Treat the bytes as untrusted data from a build-input perspective: validate their length, version, and format before interpreting them, and record their provenance.

Embedding data does not necessarily guarantee where the linker places it, whether it resides in ROM or RAM, or whether a given MCU can fetch it efficiently. Such placement is governed by target toolchain and linker configuration. Provide a generated C array fallback for compilers without the directive.

## 19.15 Static assertions expose assumptions early

A compile-time assertion can check that an integer width, structure-related condition, or application constant meets a documented requirement. It cannot check a runtime packet's length or prove that an allocation will succeed.

C23 accepts `static_assert` directly as a keyword. To detect a feature's availability, compile a tiny representative translation unit with the intended compiler flags rather than assuming support from a marketing claim about C23 compliance.

## 19.16 Build diagnostics as a teaching aid

The book's `compile_time.c` demonstrates simple macro expansion and C23 initialization. Add variations that intentionally fail `static_assert` in a scratch file and observe the compiler's message. Then compare that translation-time diagnostic to a runtime check that reports an error without preventing compilation.

**Further problems:** Explain the difference between `STR_DIRECT(VERSION)` and `STR_EXPANDED(VERSION)`. Implement a portable fallback for an embedded byte array. Design a compile-time condition selecting one of two platform adapters and fail explicitly when neither is supported.
