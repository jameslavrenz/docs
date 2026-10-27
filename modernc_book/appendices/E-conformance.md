# Appendix E. Conformance and Migration Checklist

This appendix separates three questions commonly confused in modern C work: **What does C23 specify? What does the compiler implement? What does the target environment provide?**

## E.1 Identify the build

Record the C compiler vendor, version, language-mode option, target triple, C library version, linker and startup/runtime configuration. A hosted GCC build on a development machine does not establish the capabilities of an Arm cross-compiler paired with a reduced runtime library.

A language-version probe can inspect `__STDC_VERSION__`. Its C23 value is `202311L`, but this alone does not prove that every new library facility is available.

## E.2 Core-language probes

Use tiny independent translation units to test the exact construct you plan to depend on. Features worth testing separately include:

- C23 boolean and null-pointer keywords: `bool`, `true`, `false`, `nullptr`.
- Empty initialization with `{}`.
- Binary integer literals with `0b` and quote digit separators.
- `auto` deduction, `typeof` and `typeof_unqual`.
- `constexpr` objects.
- `_BitInt(N)` within the required target width.
- Standard attribute syntax.
- The `#embed` directive for required binary resources.
- `#elifdef` and `#elifndef` conditions.

A successful syntax test is not an execution test. For numerical and representation-sensitive features, execute a small representative test on the target when possible.

## E.3 Standard-library probes

C23's `<stdckdint.h>` and `<stdbit.h>` deserve independent include-and-link checks. A compiler may accept `-std=c23` while the selected C library does not provide them. The same caution applies to optional threading and atomic facilities in a freestanding environment.

For the checked arithmetic library, write a probe that calls `ckd_add` with representable and overflowing operands. For bit utilities, test a representative `stdc_count_ones` use. Avoid assuming availability because a header file with the same name happens to exist; compile and link the intended operation.

## E.4 Representation assumptions

Record `CHAR_BIT`, integer widths, alignment requirements, and any assumptions about floating-point formats. Use static assertions only where the application requires the constraint. A text-only algorithm need not reject machines with unusual byte widths simply because a different example in the book requires eight-bit octets.

An external protocol should define field widths, byte order, and validation rules. Do not serialize native structs by writing `sizeof(struct)` bytes unless the external format explicitly specifies the exact local ABI as its contract.

## E.5 A practical migration order

1. Build the existing code under its original supported standard and save test evidence.
2. Enable warnings and fix invalid declarations, mismatched prototypes, and ignored errors.
3. Audit signed arithmetic, narrowing conversions, pointer lifetime, and buffer-size calculations.
4. Remove reliance on old-style function declarations and other obsolescent assumptions.
5. Move to C23 mode without changing algorithms.
6. Introduce modern spelling and features selectively where the tested toolchains support them.
7. Run differential tests against the original behavior for valid inputs.
8. Validate on each real embedded target and document any implementation-defined dependencies.

A source-level conversion that compiles is not a complete migration. The program's runtime contract, memory usage and timing must still be verified.

## E.6 Deployment acceptance checklist

Before accepting a release, confirm that the build uses the intended source commit; all shipped features passed their targeted probes; test inputs cover boundaries and failure cases; the compiler and linker warnings were reviewed; sanitizer results, where available, are recorded; and hardware-dependent code was exercised on real hardware.

The document build has its own acceptance criteria: all chapters and appendices appear, headings render in the correct hierarchy, the contents links work, code listings are legible, and the PDF's page count is measured directly. A PDF file existing on GitHub does not by itself prove that it corresponds to the latest manuscript commit.

## E.7 Authoritative resources

- ISO/IEC 9899:2024, *Programming languages — C* (normative reference)
- https://en.cppreference.com/w/c/23 — overview and links to individual language/library topics
- https://gcc.gnu.org/projects/c-status.html — GNU C standard support
- https://clang.llvm.org/c_status.html — Clang language support

Library and compiler documentation change over time. Date your conformance matrix and retest after updating any part of a deployment toolchain.


## E.8 Reproducible C23 implementation-support probes

The core executable examples in this book deliberately avoid requiring every advanced C23 facility. Their use is optional for the book's examples; this does not mean that the underlying facilities are necessarily optional provisions of ISO C23. The repository also provides `scripts/probe_c23_features.py`, which can test selected features individually under a named compiler.

Run it with the same language mode, compiler, and standard library that the intended application will use:

```sh
cd modernc_book
CC=gcc-14 python3 scripts/probe_c23_features.py
CC=clang-18 python3 scripts/probe_c23_features.py
```

The exact executable names and versions may differ on another host. These commands build and run small programs; cross-compilation for an MCU may require a target runner instead of executing binaries on the build machine.

The current probe set tests five independent facilities:

| Probe | Layer being tested | Why an application might use it |
| --- | --- | --- |
| `constexpr` scalar object | Compiler's C23 language front end | Translation-time named constants |
| Nine-bit `_BitInt` | Compiler and selected target | Integers with explicit value-width requirements |
| `<stdckdint.h>` checked addition | Header plus compiler/library integration | Overflow-tested integer operations |
| `<stdbit.h>` bit-count operation | Header plus compiler/library integration | Portable bit-width and population-count utilities |
| `#embed` with two known octets | Preprocessor and build resource search | Inclusion of small binary resources |

Each probe reports whether the feature actually compiled, linked, and produced its expected result on that particular host. The report does not declare all C23 features implemented merely because one succeeds.

An unsupported probe is recorded rather than failing the separate basic-example regression suite. This is intentional: the ordinary examples should remain usable on systems without a particular optional library facility. A *project that depends on the missing facility* must still reject that toolchain or select a carefully tested alternative implementation.

### Observed GitHub Actions results — October 27, 2026

The following table records **actual compile-and-run probes** in the repository's Ubuntu 24.04 CI environment. It applies to these exact hosted compiler setups, not to every build or target using the same compiler family.

| Tested facility | GCC 14 | Clang 18 |
| --- | --- | --- |
| C23 `constexpr` scalar object | Passed | Not accepted |
| C23 nine-bit `_BitInt` | Passed | Passed |
| `<stdckdint.h>` checked addition | Passed | Passed |
| `<stdbit.h>` bit count | Passed | Passed |
| `#embed` small binary resource | Not accepted | Not accepted |

The corresponding GitHub Actions run is [40514510373](https://github.com/jameslavrenz/docs/actions/runs/40514510373). Its compiler logs contain the exact diagnostics. In particular, Clang 18 reported an unknown `constexpr` identifier in the test, and both compiler setups rejected the `#embed` directive. These results should not be generalized to newer Clang or GCC versions without rerunning the probes.

An application that depends on these features must set its minimum supported compiler/library environment accordingly or use an alternative implementation. The book's mandatory example suite intentionally does not depend on the two unavailable features.

## E.9 Language version and library version are different

The compile flag `-std=c23` requests a language mode. It does not guarantee a complete hosted C23 library or every optional header on all installations. The build can also select an older library than the compiler release might suggest, especially when cross-compiling to firmware targets.

Treat support as a tuple rather than a yes/no label:

```text
(compiler release, language flags, target triple,
 C library and version, startup/linker environment)
```

A probe that passes on one Linux host may fail when the same compiler targets a freestanding MCU. Even if it compiles, its runtime use may be unsuitable inside an interrupt handler or may bring in an unexpectedly large support library.

Some features are also limited by target representation. For instance, the ability to declare a bit-precise integer does not specify a stable byte-oriented packet layout. A protocol must define its own widths, byte order, and packing separately from the C type.

## E.10 Distinguish published C23 from later proposals

This edition uses ISO/IEC 9899:2024 as its language reference. Working-group papers published later may discuss improving `constexpr`, expanding constant-expression semantics, or changing other language features. Those proposals are useful context but are not retroactively part of C23 merely because they were proposed in 2026.

For example, C23's rules for `constexpr` pointer initializers are more restrictive than some later proposed extensions. Readers should consult the published standard and verified compiler documentation before treating a draft proposal as accepted syntax.

This distinction matters when evaluating online examples. Source code labeled 'modern C' may rely on a compiler extension, a forthcoming language proposal, or a feature present in a different C-family language. The relevant question for a production build is whether *the selected target toolchain* accepts it under the chosen contract.

## E.11 How to record a defensible result

A useful conformance report contains more than a green checkmark:

- The exact source revision and probe file.
- The compiler's full version and selected target.
- The language-mode and warning flags.
- The linked C library and relevant headers.
- Whether translation, linking, execution, and expected-value checks passed.
- Any fallback selected for a feature the implementation lacks.
- Which real-target measurements or runtime checks remain outstanding.

Keep compiler-supported syntax separate from application correctness. A `#embed` probe can prove that the compiler incorporated two known bytes; it does not show that a large embedded model will fit within firmware flash and RAM constraints. A checked-arithmetic test can prove that a chosen operation reports overflow; it does not establish that every application value has a valid semantic range.

The purpose of a feature matrix is to make toolchain dependencies explicit and reproducible—not to increase the number of features listed on a résumé or in a build configuration.
