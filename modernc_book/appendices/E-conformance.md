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
