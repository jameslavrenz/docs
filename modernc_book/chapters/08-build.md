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


## 8.19 Follow the module through the compiler

The repository's `examples/modules/` directory contains three files: `checksum.h`, `checksum.c`, and `main.c`. They illustrate a complete but small source-level library.

The header declares an operation that computes a byte-wise XOR of a counted input:

```c
#ifndef BOOK_CHECKSUM_H
#define BOOK_CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>

bool book_xor_checksum(const unsigned char *bytes, size_t count,
                       unsigned char *out);

#endif
```

The implementation includes this header. That makes it impossible for its function definition to silently disagree with the declaration without the compiler having the opportunity to diagnose the mismatch.

A caller also includes the same header. The compiler can then type-check the function call, even when it has not yet seen the implementation's source file.

The entire demonstration is built as a single executable in the book's main Makefile. However, its two source files can also be compiled separately to object files:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c examples/modules/checksum.c -o checksum.o
cc -std=c23 -Wall -Wextra -Wpedantic -c examples/modules/main.c -o module_main.o
cc checksum.o module_main.o -o modules_demo
./modules_demo
```

These commands are run from the `modernc_book` directory and assume a compiler named `cc` that understands C23. The final link command is what combines the two compiled translation units into a runnable program.

## 8.20 A header is a contract, not a shared implementation file

A public header normally provides declarations and the types needed by callers. Ordinary externally linked function definitions belong in implementation files. If an externally linked definition is pasted into a header included by many source files, the linker may encounter multiple definitions.

There are legitimate header-defined functions, including appropriately declared `static inline` functions and implementation-specific patterns. Those require understanding the function's linkage and emission rules. The straightforward starting design is one externally visible definition in one source file, with declarations visible through a self-contained header.

The include guard stops repeated textual inclusion within one translation unit. It does not resolve multiple definitions across different translation units. These are separate concerns.

An implementation file should generally include its own public header first. If the function signature changes accidentally, a compile-time discrepancy is detected close to the source of the mistake.

## 8.21 The difference between syntax errors and link errors

Imagine the caller includes the header and invokes `book_xor_checksum`, but the object file containing the function definition is omitted from the link. Each source file may compile perfectly well on its own. The final link step then fails to resolve the function's external symbol.

That is not a missing declaration; the declaration was present. It is a missing definition in the linked program.

Conversely, if the caller omits the header and uses an undeclared function, modern C requires a diagnostic for the invalid declaration situation. A later linker success cannot repair the source-level type contract.

Distinguishing these cases makes build failures easier to diagnose:

| Symptom | Likely stage | First investigation |
| --- | --- | --- |
| Unknown type `size_t` | Compile | Header's own dependencies |
| Conflicting function types | Compile | Declaration versus definition |
| Undefined reference to a known function | Link | Missing object or library |
| Multiple definition of a symbol | Link | Repeated external definitions |
| Program runs but output is wrong | Execution | Algorithm and input contracts |
| Works on host but not on MCU | Target integration | Runtime, ABI, drivers and linker script |

These are categories, not a complete error taxonomy; compilers and linkers use different diagnostic messages.

## 8.22 Automate dependencies rather than relying on luck

Suppose `checksum.h` changes but a build system recompiles only `checksum.c`. The old `main.o` may have been compiled using a different interface assumption. A successful link is not always enough to establish source-level agreement if objects come from incompatible revisions.

Makefile dependencies must therefore include included headers as well as source files. On many GCC-compatible toolchains, options such as `-MMD -MP` can generate dependency files listing included non-system headers. The exact flag support should be checked on the chosen compiler.

The book's small Makefile can list the three module files explicitly, avoiding a need for generated dependencies in the teaching example. Larger projects should move to reliable dependency generation or a build system that handles it.

A clean build is useful for validation, but it should not be the only way to avoid stale object files in daily work.

## 8.23 Host and target have different responsibilities

The C language defines source and translation semantics, but it does not choose a particular microcontroller linker script, reset vector, RTOS runtime, memory map, or device driver.

An embedded build can involve:
- a cross-compiler targeting a different instruction set;
- startup code that initializes memory before calling application entry points;
- a linker script locating code and data in flash and RAM;
- a chosen implementation of the C library, sometimes with reduced functionality;
- hardware-specific interrupt and peripheral definitions;
- a flashing and debugging mechanism.

A C module can remain portable even while the final firmware image is highly platform-specific. Isolate target details in separate adapters and keep the module's public contract independent of register addresses where possible.

A hosted `FILE *` test does not prove that the firmware image has a filesystem, and a hosted `<threads.h>` test does not prove that the MCU toolchain supplies C threads. Test those interfaces separately.

## 8.24 Feature tests must include the library

A compiler accepting `-std=c23` does not guarantee that its selected C library provides every new C23 header. A feature probe should include the intended header, use a representative interface, compile, link, and when meaningful, run the result.

For `<stdckdint.h>`, a probe calls `ckd_add` on fitting and overflowing operands. For `<stdbit.h>`, it calls the desired bit utility. For `#embed`, a probe includes a small known binary resource and verifies its compiled contents. Those are different checks because some features are language-front-end functionality and others depend on the library or build resource handling.

Do not silently downgrade a test to C17 and still report that it verified C23 support. Record the exact dialect, compiler, library, target, and observed result.

## 8.25 Reproducible does not mean identical machine code

Two conforming compilers may generate different instructions from the same C program while preserving its required observable behavior. Different optimization levels, ABIs, standard libraries and linker configurations also affect the resulting executable.

A reproducible software workflow should identify its source revision, toolchain and options so an observed build can be repeated. Bit-for-bit reproducible binaries require additional controls, such as deterministic archives, build paths, timestamps and toolchain versions.

For this book, the immediate goal is narrower but important: each example should compile and execute under the specified C23-capable CI compilers, and the PDF should be generated from the intended manuscript revision.

## 8.26 A teaching exercise in failure classification

Deliberately introduce one fault at a time into the checksum module:

1. Remove `<stdbool.h>` from the header and compile a client that includes it first.
2. Change the function's output pointer type in the implementation only.
3. Omit the implementation object when linking the client.
4. Change the algorithm to XOR one byte fewer than required.
5. Change the source's accepted null-pointer contract but leave its tests unchanged.

For each, record whether compilation fails, linking fails, execution fails a test, or a contract mismatch remains hidden by incomplete tests.

This exercise emphasizes the limits of each stage. Compilers catch some interface errors. Linkers resolve definitions. Runtime tests check selected behaviors. Code review and specifications establish the requirements against which those behaviors should be judged.

### Further exercises

**8.12** Compile the module into separate object files and link them by hand. **8.13** Make a header self-contained by including its required standard type definitions. **8.14** Add a generated header-dependency file for a two-source project. **8.15** Create separate host and embedded build configurations without changing the checksum algorithm. **8.16** Document the difference between a missing function declaration and a missing function definition.
