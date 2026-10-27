# 8. The Preprocessor and Separate Compilation

The C preprocessor transforms translation units before compilation. It helps share declarations and configure programs, but its token-level substitutions must not be mistaken for typed functions.

## 8.1 Headers describe interfaces

```c
/* counter.h */
#ifndef COUNTER_H
#define COUNTER_H

#include <stddef.h>
struct counter { size_t value; };
void counter_increment(struct counter *c);

#endif
```

```c
/* counter.c */
#include "counter.h"
void counter_increment(struct counter *c)
{
    if (c != nullptr) ++c->value;
}
```

The header is included by both the definition and callers, allowing the compiler to check their consistency. The include guard prevents repeated definitions within a translation unit.

## 8.2 Prefer typed operations

```c
#define SQUARE(x) ((x) * (x))
```

This macro evaluates its argument twice; `SQUARE(i++)` produces undefined behavior due to unsequenced modifications. A function avoids repeated evaluation:

```c
static inline int square(int x) { return x * x; }
```

The function has one parameter evaluation, though signed multiplication can still overflow. `inline` is not a promise that machine code will contain no call.

## 8.3 Conditional compilation

```c
#if defined(DEVICE_TARGET)
    /* device-specific interface */
#else
    /* host implementation */
#endif
```

Keep conditional code small and localized. C23 adds `#elifdef` and `#elifndef` as clearer forms for common conditional cases. Feature-test macros should verify actual support rather than assume the entire standard library is implemented because a compiler accepts `-std=c23`.

## 8.4 Compile and link

For two files:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c counter.c
cc -std=c23 -Wall -Wextra -Wpedantic -c main.c
cc counter.o main.o -o program
```

The compilation stage separately translates source files; the linker resolves symbols and constructs the executable. An undefined reference usually means the program declared a symbol but no linked object provided its definition.

**Exercises.** 8.1 Write a minimal `Makefile` for the two-source example. 8.2 Replace a parameterized macro with an inline function. 8.3 Explain the distinction among declaration, tentative definition, and external definition.


## 8.5 What belongs in an interface?

A header exposes names and types necessary for clients. A private source file keeps the implementation's helper functions, internal objects, and algorithms. Public headers should be self-contained: including one in an otherwise empty translation unit should not require secret prior includes.

A useful test compiles a short file containing only the header and a minimal `main`. This catches hidden dependencies that may otherwise be masked by the include order in a larger build.

## 8.6 Macros and evaluation hazards

Function-like macros are token substitution, not calls. This matters for evaluation count, operator binding, statement boundaries, and type checking. Compare:

```c
#define MIN_BAD(a, b) ((a) < (b) ? (a) : (b))
static inline int min_int(int a, int b)
{
    return a < b ? a : b;
}
```

The macro may evaluate one argument twice, while the function evaluates each call argument once. Even the function does not specify which argument expression is evaluated first; do not write code that depends on call-argument evaluation order.

## 8.7 Build modes and reproducibility

A program's observed behavior depends on source code, implementation, target, optimization settings, language standard, and linked libraries. Keep warning flags in normal development builds. Use optimization during testing as well as debugging, since undefined behavior may appear differently at different optimization levels.

The C standard does not dictate a build system. A small Makefile can teach separate compilation effectively. Larger projects benefit from explicit dependency tracking, toolchain files, and reproducible CI environments.

## 8.8 Defensive configuration

Prefer feature tests to compiler-name checks. Detecting a compiler vendor does not prove that a header is available or that a function links with the intended C library. For a C23 library feature, compile and link a minimal program using it before enabling the code path. Document fallbacks for older supported targets.

## 8.9 Exercises

**8.4** Write a header-only compile probe. **8.5** Show why a macro with arguments containing `++` can change program behavior. **8.6** Build a two-file project with a separate `-c` step and explain a linker error. **8.7** Define what toolchain details must be recorded to reproduce a firmware build.


## 8.10 A source file is not an isolated program

Suppose one source file includes a header declaring `int calculate(int);`, and another defines `double calculate(double)`. Each source can appear locally plausible, but the program violates the intended cross-file interface. Include the same header in the implementation file, build with warnings enabled, and avoid hand-copied prototypes.

At link time, the toolchain resolves external references. A linker error about an undefined reference can mean the implementation object was not linked, the identifier has internal linkage, or the symbol spelling does not match. Distinguish such linkage errors from a compiler diagnosis about a malformed declaration.

## 8.11 Build a tiny library deliberately

A three-file project can contain `counter.h`, `counter.c`, and `main.c`. Compile the implementation and main program into separate object files, then link them. In a Makefile, express dependencies so a changed header causes both translation units to rebuild. A project that rebuilds only the implementation after changing a public structure definition may link stale object code with an incompatible layout.

For larger projects, generated dependency files or a build system such as CMake can automate this bookkeeping, but understanding the underlying dependency graph remains valuable.

## 8.12 Host and target are different builds

An embedded program may compile once for a desktop host and once for an Arm microcontroller. The compiler, target ABI, linker script, startup code, libraries, and permissible memory layout can be different. A program's `sizeof(long)`, pointer representation, object alignment and supported atomics must not be inferred from the host build.

Keep portable algorithms in files that both environments can compile. Isolate peripheral drivers and target-specific register definitions behind narrow interfaces. This makes host-side unit tests useful without pretending they replace hardware validation.

## 8.13 Preprocessor conditions versus feature probes

A macro such as `__STDC_VERSION__` can tell you which language revision an implementation claims for a translation mode. It cannot prove that optional facilities or the desired target C library are available. To depend on `<stdckdint.h>`, compile and link a small test that actually includes the header and calls a checked arithmetic operation. To depend on `#embed`, compile an actual embedding directive using the chosen toolchain.

The distinction matters because a C front end and its runtime library are often delivered separately. Firmware toolchains may ship specialized or partial libraries.

## 8.14 Keep generated output reproducible

The manuscript's PDF is generated from Markdown. A reliable release records the source commit, document toolchain versions, fonts, and build command. The page count and layout can change when these vary. Committing a PDF without a verified successful build may give readers a stale document that does not match the latest chapters.

Use a CI workflow to build, inspect, and publish the PDF after source changes, with an explicit failure whenever required tools or referenced files are missing. An automation configuration in a repository is a plan for a build, not evidence that the build actually passed.

**Further problems:** Write Makefile rules for two object files sharing a header. Explain the meaning of `-c`. Design a two-compiler CI matrix and identify how to distinguish compiler-support failure from a programming error.

## 8.15 A real three-file C module

A header becomes useful when more than one translation unit agrees to an interface. The companion example under `examples/modules/` consists of a header, an implementation, and a separate client program. It computes a checksum over a bounded byte span. The calculation is intentionally elementary; the lesson is how declarations, definitions, and build dependencies cooperate.

The header contains the declaration:

```c
#ifndef BOOK_CHECKSUM_H
#define BOOK_CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>

bool book_xor_checksum(const unsigned char *bytes, size_t count,
                       unsigned char *out);

#endif
```

The implementation includes that same header, rather than independently restating the function's type. It also places any helper functions under internal linkage with `static`. The client includes the header and calls the function. The build compiles both source files and links their object code.

Why insist that the implementation include its own header? Suppose a developer changes the parameter from `size_t` to `unsigned` in only the implementation. Without the header, different translation units could compile against incompatible assumptions. With a common header, the compiler can diagnose disagreement where the function is defined.

## 8.16 Rebuild dependencies

A Makefile must know more than the final executable name. If `checksum.h` changes, both the implementation that defines the operation and the client that calls it may need recompilation. For a small demonstration, an explicit recipe that depends on all three files is sufficient:

```make
modules_demo: examples/modules/main.c examples/modules/checksum.c examples/modules/checksum.h
	$(CC) $(CFLAGS) examples/modules/main.c examples/modules/checksum.c -o $@
```

For larger projects, compile to separate object files and generate header dependencies automatically. Otherwise a stale object file may be linked against an updated structure layout or changed interface.

This particular checksum is not a cryptographic digest. It is a teaching example for module boundaries and input-length contracts. Using it for message authentication would be a security design error.

## 8.17 Header hygiene and include order

A well-designed public header should compile when included first in an otherwise minimal translation unit. It should include the standard headers needed by its own declarations instead of relying on the client to include them beforehand.

Include guards prevent repeated textual inclusion within one translation unit, not multiple external definitions across separate source files. Define ordinary global storage in one implementation file; put only an `extern` declaration in the header if clients truly need to refer to it.

An API can often avoid exporting global state at all. Pass a context structure or opaque handle through function arguments so tests can create independent instances without accidental interference.

## 8.18 Libraries are composed, not merely named

A static library usually packages compiled object files into an archive. A shared library adds platform-specific dynamic-linking rules and lifetime constraints. The C language itself specifies translation and linkage concepts but does not require a particular operating-system library format.

An embedded firmware image may link application code, an RTOS, a runtime library, startup code and a linker script in one final step. The C code can be portable while its final link process is highly target-specific.

### Exercises

**8.8** Compile the module example with separate `-c` steps. **8.9** Remove the implementation file from the link command and explain the resulting diagnostic. **8.10** Delete `<stddef.h>` from the header and determine why standalone inclusion now fails. **8.11** Add a unit-test client using the same header but a different `main`.
