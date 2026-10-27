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
