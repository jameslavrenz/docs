# 13. Testing, Diagnostics, and Portability

A C program can compile successfully and still be wrong. Compiler diagnostics report only what a tool can establish from the program and its selected options. Tests sample behavior for chosen inputs. Static analysis may identify suspicious paths. Sanitizers instrument executable operations. None of these alone proves that a C program is correct, but used together they provide an effective development discipline.

This chapter develops a concrete workflow for the small programs in this book.

## 13.1 Start with the contract

Before building a test, state what success means. Suppose a function computes the sum of a sequence of unsigned integers. Its contract might say:

- The input is a pointer to at least `count` elements, or a null pointer if `count` is zero.
- The result pointer must address writable storage.
- On success, the output is the exact mathematical sum.
- On failure, the output is unchanged.
- Overflow is an error rather than a request for modular arithmetic.

These statements determine useful tests. They also identify cases that a test **cannot** legitimately exercise: passing a dangling pointer or claiming a buffer has more elements than it actually has violates the caller's contract. Running such a case and observing a crash does not constitute a useful portable API result.

Tests should therefore separate *valid edge cases* from intentional demonstrations of undefined behavior.

## 13.2 Organize tests by dimensions

For a bounded operation, look for boundaries in several dimensions:

| Dimension | Representative tests |
| --- | --- |
| Count | zero, one, capacity minus one, capacity, capacity plus one |
| Value | zero, ordinary value, largest representable value |
| Text | empty, valid, invalid digit, too long, no trailing newline |
| Storage | sufficient capacity, insufficient capacity, allocation failure |
| State | empty collection, full collection, wraparound, repeated use |
| I/O | clean EOF, short record, read error, write error |

A useful test does not merely show that a function returns the expected value. It checks all promised postconditions. If failure must preserve an output, initialize it to a recognizable value before invoking the function, then verify it remains unchanged.

## 13.3 Assertions for tests, not input validation

The `assert` macro in `<assert.h>` checks a condition when assertions are enabled. Failed assertions terminate the program with a diagnostic. Defining `NDEBUG` can disable them.

```c
#include <assert.h>
#include <stddef.h>

static int first(const int *values, size_t count)
{
    assert(values != nullptr && count != 0);
    return values[0];
}
```

This is appropriate only if callers are required to satisfy the precondition and an assertion failure is an acceptable development-time response. It is not a substitute for checking user-supplied input in a production interface. A release build may omit the assertion altogether, after which a bad call still attempts the invalid access.

Never place an operation with required side effects only inside `assert`; disabling assertions would also remove that operation.

## 13.4 A test harness without external libraries

For a small project, a conventional `main` that returns nonzero on failure is enough:

```c
#include <stdbool.h>
#include <stdio.h>

static bool is_even(unsigned n)
{
    return n % 2U == 0;
}

int main(void)
{
    if (!is_even(0) || !is_even(8) || is_even(7))
        return 1;
    puts("tests passed");
    return 0;
}
```

The build system can run the executable and use its exit status to determine success. Larger suites should report the particular case that failed, but a simple test program can be both readable and valuable.

A command such as `printf 'a b c' | ./word_count` tests a whole program through its standard input and output. Such integration tests complement direct function tests, which are better at reaching internal boundary cases.

## 13.5 Warnings and language modes

A practical first compiler invocation is:

```sh
gcc -std=c23 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Werror -O2 example.c -o example
```

The exact useful flags vary by compiler, codebase, and target. In particular, `-Wconversion` can be noisy in legitimate numeric work, and `-Werror` can cause builds to fail when the compiler introduces new diagnostics. The correct response is to review warnings and make an explicit project policy, not to silence them indiscriminately.

Compile in the actual language mode. A compiler accepting one C23 keyword does not necessarily implement every feature or library facility. When a project supports more than one compiler, build with both. Different diagnostics often reveal different classes of mistakes.

## 13.6 Sanitizers

On supported host targets, AddressSanitizer can detect many out-of-bounds accesses, use-after-free errors, and other memory defects. UndefinedBehaviorSanitizer can detect selected undefined operations, such as certain signed overflows or invalid shifts. They do not cover every form of undefined behavior, and their effectiveness depends on executing the erroneous path.

A useful build on a supported GCC or Clang environment is:

```sh
cc -std=c23 -O1 -g -fno-omit-frame-pointer \
   -fsanitize=address,undefined example.c -o example_san
./example_san
```

Sanitizer-enabled executables are typically host-oriented. Their instrumentation, runtime overhead, and dependencies can make them unsuitable for microcontrollers. Test algorithms on a host, then separately verify target-specific assumptions.

Do not interpret one clean sanitizer run as proof of a fully safe program. Test case selection remains decisive.

## 13.7 Unit tests and property tests

A unit test checks a specific expected value. A *property test* checks a relationship that should hold for many inputs.

For example, consider an in-place reversal of an array. If the operation is correct, reversing twice should restore the original sequence. The property holds for empty arrays, one-element arrays, and arrays with duplicates. A test driver can generate random arrays and check this relationship.

Similarly, if a serializer and parser implement the same format, decoding an encoded valid value should produce an equivalent value. This does not prove either routine matches an external specification: they could share the same mistake. Include independent golden test vectors alongside round-trip tests.

## 13.8 Fuzzing a parser

A fuzzer supplies many automatically generated byte sequences and observes whether the program crashes, violates a memory safety check, hangs, or triggers an assertion. A parser interface like

```c
bool decode_packet(const unsigned char *bytes,
                   size_t length,
                   struct packet *result);
```

is well suited to fuzz testing because its input is a bounded byte sequence. The harness should supply valid storage of the declared size, initialize output state, invoke the parser, and verify that failure preserves the output.

Fuzzing discovers bugs; it does not establish absence of bugs. Use corpus inputs for known valid packets and edge cases, and preserve any newly discovered failing inputs as deterministic regression tests.

## 13.9 Compare optimization modes

Undefined behavior can cause surprising differences between `-O0` and `-O2`. A compiler is permitted to optimize under the assumption that no undefined operation occurs. It may therefore eliminate checks that seem defensive if the source has already performed an invalid operation.

When debugging a difference between builds, investigate lifetime, aliasing, integer overflow, uninitialized objects, sequencing, and incorrect function declarations. Do not rely on a particular assembly listing as the definition of C semantics.

## 13.10 Run the actual target

A host test cannot substitute for hardware validation when code depends on interrupt timing, cache coherency, memory-mapped registers, DMA, alignment restrictions, an RTOS, or the implementation's floating-point behavior.

The most effective architecture separates portable algorithms from target-specific adapters. For a firmware project, a packet decoder can be tested exhaustively on the host while the UART driver is validated on the real board. Integration tests must then establish that the adapter passes the right byte sequences and lengths to the decoder.

## 13.11 Document verified claims

A verification report should record the compiler and version, C library if relevant, command line, target architecture, test suite, and result. State the difference between *committed test code* and *tests actually executed*. If a test was not run, do not write that it passed.

For this manuscript, `make check` invokes a shell-based regression suite, while a GitHub Actions workflow can provide repeatable hosted checks. Both should be run on the precise committed revision before a stable release is declared.

## 13.12 Exercises

**13.1** Add a test for unsigned-sum overflow that verifies the output argument remains unchanged.

**13.2** Write a reversibility property test for a bounded array reversal.

**13.3** Create a fuzzer harness that invokes the packet decoder on random byte sequences and checks output preservation.

**13.4** Compare warning diagnostics on the same example under GCC and Clang. Explain differences without suppressing them globally.

**13.5** State which properties of a ring buffer can be established with single-threaded tests and which need concurrency-specific reasoning.

**13.6** Design a verification matrix for a program that must run on an x86-64 development host and an ARM Cortex-M microcontroller.


## 13.13 Make tests deterministic

A regression test should produce the same expected result from the same inputs under documented conditions. Tests based on timing thresholds, random seeds without recording them, or unspecified evaluation order are difficult to reproduce. When randomness is useful, record the seed and provide a way to replay the failing case exactly.

For the book's word index, generate a fixed set of vocabulary and expected counts. For the bounded queue, push and pop a known sequence through wraparound. For the byte decoder, use independent golden vectors defined by the protocol rather than relying only on round trips.

## 13.14 Test failures, not just successes

Many defects hide on error paths. Test the allocation path that fails, the input that is one byte too short, the array that is exactly full, and the output that must not change when conversion fails.

A useful method is *fault injection*. A test allocator can fail on the third request; a fake byte source can return an I/O error after a chosen number of bytes; a fake output sink can accept only a prefix of a write. The production algorithm should treat these failures as ordinary results.

When an API promises strong failure safety, initialize the output with an unusual value and verify it remains unchanged after every rejection. Be careful not to compare structure padding bytes with `memcmp` and interpret differences as changes in member values.

## 13.15 Static analysis and sanitizers complement each other

A static analyzer inspects source or intermediate representation without necessarily running every path. It may detect impossible conditions, potential null dereferences, and unchecked return values. False positives exist, and the analyzer's assumptions must be understood.

AddressSanitizer can catch many invalid memory accesses when they execute. UndefinedBehaviorSanitizer can catch selected invalid shifts, arithmetic, and other errors. Neither usually establishes absence of data races; ThreadSanitizer is a different instrument and has platform-specific limitations. Sanitizers also change timing and memory layout, so embedded hardware behavior still requires separate validation.

For numerical kernels, compiler warnings and differential tests with independent reference implementations may reveal issues that memory sanitizers cannot.

## 13.16 Differential testing

When implementing a custom parser, compare its accepted strings and decoded values with a second trusted implementation **after matching their grammars**. If one parser allows leading whitespace and the other forbids it, disagreement may be by design rather than a bug.

For a numeric function, compare against a reference with higher precision or a well-understood library routine over representative ranges. For a protocol codec, compare byte-level outputs against test vectors written from the format specification.

A test that encodes and decodes using two functions sharing the same mistaken endianness can pass while violating the external protocol. Independent expectations are essential.

## 13.17 CI as reproducible evidence

A GitHub Actions job should report its actual compiler and version, the selected dialect, and the command that ran. Separate compilation, unit tests, sanitizer builds and document generation into distinguishable steps so a failed PDF toolchain is not mistaken for failing C code.

A CI configuration file is not itself proof of a passing build. Evidence requires inspecting an actual completed workflow run for the exact commit. If the repository cannot expose run status through the available integration, say that verification is pending.

## 13.18 Choose a release criterion

A reasonable first reading edition can require all examples to compile under two supported compilers, all deterministic tests to pass, all Markdown links to resolve, and the generated PDF to include every chapter and appendix. A publishable technical book needs more: technical review, a coherent index, verified page layout, exercise solutions, and clear statements of compiler-specific features.

**Further problems:** Design a differential test for a little-endian decoder using independent literal expected values. Explain why a clean sanitizer run cannot prove every array index is valid. Add a fault-injectable allocation layer to a dynamic vector and report which postconditions remain true.

## 13.19 A test is a contract experiment

Testing becomes more systematic when each case is derived from a documented precondition, invariant, or postcondition. For a vector, the invariants include `length <= capacity` and valid ownership of allocated storage. For a packet parser, failure must not modify the public result. For an SPSC queue, the producer and consumer must observe values in FIFO order under the single-producer/single-consumer protocol.

A test should state what it establishes. A successful sequential queue test demonstrates ordering and wraparound for the tested operations. It does not establish freedom from data races if producer and consumer execute simultaneously. A clean AddressSanitizer run shows that the tested execution did not trigger any instrumented errors; it does not prove all paths safe.

## 13.20 Use fault injection to reach unlikely branches

If a container allocates only a few dozen bytes, ordinary development runs rarely experience allocation failure. A design that can accept a resizing callback allows a test to deliberately return `nullptr` and verify strong failure safety. The book's `vector_checked.c` follows this pattern.

An analogous technique works for output interfaces. The complete `callback_sink.c` example supplies a fake sink whose write operation accepts at most two bytes per call. A `write_all` wrapper must retry the unwritten suffix until the whole buffer has been accepted. A second fake mode returns no progress, and the wrapper must report failure rather than loop forever.

These are not theatrical failures. Embedded UART queues can fill, filesystem writes can fail, network links can stall, and bounded buffers can run out of capacity. Making errors injectable produces reproducible tests for behavior that otherwise appears only in the field.

## 13.21 Test the smallest complete interface

A function-level test can verify a single boundary, but a project also needs tests for interfaces between modules. The module example in Chapter 8 exercises compilation through a header, implementation, and client. The word-index test exercises the complete command-line program through standard input and output. The PDF build tests documentation generation separately from C compilation.

Keeping these levels distinct helps diagnose failures. If compilation fails because of a C23 syntax diagnostic, rewriting a parser algorithm is unlikely to help. If a program compiles but returns the wrong table, investigate semantics and test data. If the PDF builder fails while C tests pass, investigate the document toolchain.

## 13.22 Measure coverage without worshipping a number

Line and branch coverage can reveal unexecuted paths, especially error-handling branches. They do not say whether a test checks the right result. A test might run the overflow check but ignore the output state afterward. A branch can be covered without verifying that it preserves a container's ownership contract.

For high-risk functions, review a small table of input classes: minimum size, maximum size, invalid value, missing data, exhausted resource, and valid boundary. Include a human explanation of why each case matters.

## 13.23 Reproducible CI

A useful CI run records the exact commit, compiler versions, flags, and exit status. Different compilers discover different issues, but changing to a newer toolchain can also change diagnostics. When the compiler misclassifies valid C23 syntax as an extension, handle that specific diagnostic rather than globally disabling `-Werror`.

For publication, add a second gate: the generated PDF must contain every chapter and appendix, have a readable table of contents, and be built from the same source revision being published. A source repository containing an older PDF should not be treated as proof that the latest manuscript has been typeset.

### Exercises

**13.7** Write failure-injection cases for an allocator and a short-writing output sink. **13.8** List what a passing host test cannot establish about MCU interrupt timing. **13.9** Add a CI check that separately reports compiler-test and PDF-build failures. **13.10** Design independent golden vectors for a three-byte header codec.
