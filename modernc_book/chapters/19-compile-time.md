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

## 19.17 A preprocessor macro is not a generic function

A function-like macro may evaluate an argument more than once, and even a macro wrapped in parentheses does not become type-safe. The familiar `MAX(a, b)` substitution illustrates the danger when a caller passes `index++`. The expression chosen by the conditional operator may be evaluated again.

A `static inline` function for a specific type avoids duplicate evaluation of its arguments. The caller's argument expressions are still evaluated in an order that C does not necessarily prescribe, so do not write calls that depend on sequencing between them.

A `_Generic` selection can choose among several typed functions at translation time, allowing a type-directed wrapper without converting every value to one common type. Be careful to specify supported types and a clear policy for unsupported arguments.

## 19.18 Use static assertions for real assumptions

A portable byte decoder may not need to assume an eight-bit byte if it operates on abstract values in the standard character domain, but an octet-specific wire codec does need an explicit policy about eight-bit units. A compile-time assertion can make that dependency visible to future maintainers.

Do not scatter assertions about irrelevant architecture properties throughout a general library. A function operating on `size_t` lengths and opaque pointers may be perfectly valid on targets with different pointer widths, whereas a hardware register interface can have genuine address and alignment requirements.

## 19.19 Configuration is part of the reproducible build

A macro such as `DEVICE_TARGET` changes which tokens are translated. Therefore it can change program behavior just as surely as a source edit. Record the selected configuration, compiler version, flags, and target alongside test results.

For embedded code, a host adaptation may use a fake device sink, while the MCU build uses a UART driver. The public interface should remain the same; conditional compilation should select the adapter, not duplicate the entire application algorithm.

## 19.20 Keep the generated book reproducible

The book's PDF is also a build artifact. A manuscript change should trigger generation from the current source and a check that required headings appear in the output. A page count should be measured from the PDF itself, not inferred from the Markdown word count.

The build is only one acceptance gate. A PDF may technically compile but have broken line wrapping, duplicated section numbering, or code examples that run off the page. The final editorial pass requires inspecting rendered pages and checking the technical content independently.

### Exercises

**19.7** Write a two-level stringizing macro and demonstrate its expansion with a numeric macro. **19.8** Compare a macro and a typed inline function when passed an increment expression. **19.9** Design a compile-time configuration that selects a host or target byte sink. **19.10** Explain why a successful document build is not proof that every C listing compiles.


## 19.21 Generic selection is type-directed dispatch

C offers a limited but useful compile-time mechanism for choosing an expression based on the type of another expression. The `_Generic` selection form can select between named functions:

```c
static int maximum_int(int a, int b)
{
    return a > b ? a : b;
}

static long maximum_long(long a, long b)
{
    return a > b ? a : b;
}

#define max_same_type(a, b) \
    _Generic((a), int: maximum_int, \
                  long: maximum_long)((a), (b))
```

This macro is intended for two arguments of the same supported type. If the first is `int`, the generic selection chooses `maximum_int`; if it is `long`, it chooses `maximum_long`. An unsupported first-argument type produces a constraint diagnostic because no selection matches and no default association is supplied.

The selected result is a function designator; the arguments are then supplied to that function as in a normal call. The controlling expression is not evaluated at runtime merely for the selection. This matters for side effects.

For example:

```c
int count = 0;
int answer = max_same_type(++count, 7);
```

The increment in the controlling expression does **not** happen once during generic selection and again during function invocation. It occurs as part of the actual function argument evaluation, so `count` becomes one, not two.

This contrasts with a traditional function-like macro that copies argument text into both sides of a comparison and may evaluate the same argument repeatedly.

## 19.22 Generic selection still needs a contract

It is tempting to call a type-directed macro fully type-safe. But a selection based only on the first argument does not force the second argument to have that same type.

A caller might write `max_same_type(1, 2L)`; the `int` function would be selected, and the second argument would be converted to `int` according to the call's function prototype. If its value is out of range for `int`, the conversion may not produce the intended result.

A production helper may need a stronger interface, more specific functions, or a carefully designed second type check. Alternatively, it can state explicitly that callers must provide equal supported types and rely on compiler conversion warnings and code review.

The central rule is that `_Generic` selects an expression based on type. It does not invent a new language-level overloaded function system with the same constraints as C++ templates.

## 19.23 What the generic associations mean

A selection can include more types and a `default` association. It must not contain multiple compatible type associations in a way prohibited by the language's constraints.

The selection's controlling expression is used for type matching, not for a dynamic switch on its current numerical value. Thus `_Generic(x, int: foo, long: bar)` chooses according to the type of `x`, even if `x` happens to hold a value representable in both types.

For an array or qualified expression, the controlling-expression type rules deserve particular care because the language's ordinary conversions and type compatibility can affect matching. If the distinction between two closely related types is essential, compile and test the exact declaration under the targeted compiler mode rather than reasoning from spelling alone.

A small feature probe can use `static_assert` and a constant generic selection to document an expected match in the build:

```c
static_assert(_Generic((int)0, int: 1, default: 0));
```

That statement checks a translation-time property. It does not execute the selected program function.

## 19.24 Why use real functions behind the macro

The generic macro can select functions defined in ordinary C. Those functions have normal parameter types, normal prototypes, and ordinary evaluation semantics. A developer can set a breakpoint inside one, test it directly, or reuse it without the generic dispatch.

This reduces the risks of macros that implement their entire algorithm through textual substitution. It also gives the compiler a familiar opportunity to inline or optimize a small typed function when appropriate.

The design is not a universal replacement for callbacks. Generic selection occurs at translation time based on the known type, while a callback is a runtime-selected operation. Use the former when supported types are known in source and the latter when behavior must be supplied dynamically by a caller or test.

For example, a host test that substitutes a failing allocator needs a callback or equivalent runtime seam; a math wrapper choosing between float and double functions may benefit from generic selection.

## 19.25 A small validation example

The standalone `examples/generic_select.c` contains typed maximum functions and tests for both `int` and `long`. It includes `INT_MIN\), `INT_MAX\), `LONG_MIN\) and `LONG_MAX\) comparisons without subtracting their values.

The test also increments an integer in a macro argument and confirms that the operation occurs once. This directly checks the pedagogical claim that the macro does not duplicate argument evaluation.

The test does not establish that mixed-type inputs are safe. The macro's documented caller contract requires matching supported types, so the example illustrates one constrained application of `_Generic`, not every possible selection design.

## 19.26 Macro expansions can mislead a debugger

The preprocessor operates before ordinary compilation of expressions. An object-like or function-like macro expands to tokens, not a separately callable runtime function. Error messages and debugger stepping may therefore point into the macro expansion or into a user call site rather than behave like a normal function call.

When a macro becomes complicated, first ask whether a `static inline` function would express the same logic. If the macro's real purpose is compile-time selection, keep the macro small and delegate work to typed functions.

A second source of confusion is precedence. A simple macro `#define SQUARE(x) x*x` misbehaves when passed `1 + 2`, because substitution does not introduce the parentheses a function call would. Parenthesizing parameters and the result improves such macros, but it does not solve double evaluation when the argument appears twice.

## 19.27 Compile-time contracts versus runtime checks

Several different mechanisms detect different kinds of errors:

| Mechanism | When it applies | What it can establish |
| --- | --- | --- |
| `static_assert` | Translation | A constant expression meets a required property |
| `_Generic` | Translation | Selects a matching association by expression type |
| `#if` and macros | Preprocessing | Chooses tokens using preprocessing conditions |
| Function prototype | Translation | Declared argument/return type relationships |
| Ordinary `if` | Execution | Checks runtime state and input |
| Tests and sanitizers | Execution | Observe behavior on exercised paths |

No one mechanism subsumes the others. A compile-time size assertion cannot guarantee a runtime packet has sufficient available bytes. A runtime null pointer check cannot prove that a pointer refers to a still-alive object. A generic selection cannot verify an allocator's logical ownership rules.

Use each mechanism for the property it is capable of expressing.

## 19.28 Readability wins over maximum metaprogramming

C's compile-time facilities can eliminate certain kinds of repetition, but they can also obscure meaning when used excessively. An ordinary named function is often the best interface. A small `static_assert` is useful for a real target assumption. A few lines of generic selection can simplify a family of typed operations.

Do not build a complex macro-based type system merely to avoid writing a small number of clear functions. In particular, firmware intended for multiple compilers and static-analysis tools may benefit from conservative, explicit interfaces that every tool can understand.

The best criterion is whether the change makes the program easier to audit, test, and maintain under the actual supported toolchains.

### Further exercises

**19.11** Add an `unsigned` maximum function and extend the generic selection to choose it. **19.12** Create a compile-time diagnostic for an unsupported controlling-expression type. **19.13** Investigate how a mixed `int` and `long` call is converted and design an interface that rejects it. **19.14** Replace a double-evaluating macro with a typed inline function. **19.15** Compare the role of `_Generic` with a runtime callback using the same operation on input data.
