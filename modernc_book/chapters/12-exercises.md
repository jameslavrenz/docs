# 12. Exercises and Further Reading

The best way to learn C is to predict a program's behavior before compiling it, then explain any discrepancy. Exercises should include ordinary cases, boundaries, and failure cases.

## A progression of projects

**Project A — Bounded line reader.** Implement a function that fills a caller-supplied buffer, reports bytes stored, distinguishes newline from EOF, and signals truncation. Write tests for zero-capacity buffers, one-character buffers, long lines, embedded zero bytes, and input errors.

**Project B — Integer parser.** Parse unsigned decimal text into `unsigned` without `atoi`. Reject empty input and nondigits; detect overflow before multiplying by ten or adding the next digit. Show why checking after an arithmetic overflow is too late.

**Project C — A fixed pool.** Implement allocation and release of equal-sized records from a fixed array. Reject invalid and repeated releases. Describe whether the pool can be used concurrently, and if so how it is synchronized.

**Project D — Binary message reader.** Decode a header with a version byte, 16-bit little-endian payload length, and checksum. Check all bounds before decoding. Test malformed lengths and truncated buffers.

**Project E — Word index.** Improve Chapter 11 with a hash table, sorting, and a test driver. Benchmark it using increasing vocabulary sizes and measure memory usage.

## How to study source code

Read short functions and annotate their contracts. What objects must exist at entry? What inputs are valid? Which values may overflow? What state changes after success? What state is preserved after failure? Which implementation-defined assumptions are present?

Compiler warnings are evidence to inspect, not text to suppress reflexively. Sanitizers find bugs only on executed paths; code review and carefully constructed tests remain essential.

## References

- ISO/IEC 9899:2024, *Programming languages — C* (normative specification).
- cppreference, C language reference, especially its C23 features and library pages.
- GCC C standards implementation status and command-line dialect options.
- Clang C language status and diagnostics documentation.
- The ISO C Secure Coding Rules and CERT C guidance, for additional defensive-programming perspectives.

## A note on editions

This is an independent original textbook. Classic C teaching remains valuable for its economy of examples, but 2026 readers also need accurate discussion of defined behavior, lifetime, ownership, toolchain variation, safer interfaces, and C23's additions. Future manuscript revisions should add tested solutions, further real-world worked examples, portable build checks, and an index before a complete book-length edition is declared.

# 12.1 Exercises as specifications

An exercise is complete only when it states what the program accepts, what it returns, and how failure is reported. Consider a decimal parser: what does it do with an empty string, leading spaces, a plus sign, invalid characters, and a number larger than the destination type? The interesting part of the problem is often deciding these details before coding.

## 12.2 Testing boundaries

For a bounded collection with capacity N, test insertion at sizes zero, one, N−1, and N, then one additional insertion that must fail without damaging existing records. For signed arithmetic, test the largest and smallest representable values, opposite signs, and zero. For a byte parser, test all shorter-than-required inputs.

## 12.3 Integration project: a streaming index

Read Chapter 11's sorted word index and complete the following experiments:

1. Validate a file containing zero words and a file ending with an unterminated word.
2. Check that capitalization is folded only for ASCII letters and that digits remain in the word.
3. Generate 1,024 unique words and verify the table fills without overrun.
4. Attempt one more unique word and confirm the operation fails.
5. Compare counts against an independent reference implementation for randomly generated ASCII input.
6. Measure runtime for sorted and reverse-sorted unique vocabularies to investigate insertion cost.
7. Replace the sorted array with a fixed-capacity open-addressed hash table; state the collision policy, load factor, and failure semantics.
8. Describe how a UTF-8 version would need a more sophisticated definition of words, code points, and normalization.

## 12.4 Integration project: a bounded vector

Start from `examples/vector.c`. Add `reserve`, `clear`, and `get` operations with contracts. Make failed growth preserve the old allocation and data. Test failed allocation by injecting an allocator that rejects selected requests. Discuss why assigning one vector object to another is not a deep copy.

## 12.5 Reading C with a reviewer

For every function, identify which variables represent length and capacity, which arithmetic might overflow, which objects are borrowed versus owned, whether outputs change on failure, which operations may allocate, and whether concurrent calls can safely share state. A program may be short and still require serious reasoning.

## 12.6 Challenge: a portable protocol reader

Design a packet decoder accepting `const unsigned char *bytes` and `size_t length`. A packet contains a magic byte, a version, a two-byte little-endian payload length, payload bytes, and an eight-bit checksum. Reject incomplete packets before reading fields; reject lengths inconsistent with the buffer; do not write the output structure until the checksum passes. Provide cases for exact minimum length, maximum supported payload, bad version, invalid checksum, and length overflow.

## 12.7 Challenge: concurrency

Design a producer-consumer queue for one interrupt-side producer and one thread-side consumer. State why Chapter 5's plain queue is not automatically safe under concurrent access. Choose either an RTOS synchronization primitive with appropriate interrupt semantics or a target-validated atomic design. Document memory-order assumptions and why the chosen operations are legal in both execution contexts.

## 12.8 Further reading in practice

After solving a problem, read the relevant normative language clause or trusted reference and compare it with the implementation's documentation. A standards-conforming program is not merely one that a compiler accepts; conformance involves the behavior required by the language and library specification.


## 12.9 How to write convincing solutions

A solution should include more than a working program. State assumptions, list invariants, justify any bounds arithmetic, and show representative tests. A design that accepts invalid input only because one particular compiler tolerated it is not a complete solution.

A useful submission format has five short parts: (1) contract, (2) algorithm, (3) implementation, (4) test table, and (5) limitations. In a small exercise this may fit on one page. In a hardware-related problem, also identify dependencies on the toolchain and target.

## 12.10 Progressive memory exercises

Start with a fixed array of integers. Add a function returning its count of nonzero elements. Next write a version accepting a pointer and a length. Then add dynamic allocation with a checked size calculation. Finally add a failure-injecting allocator and verify that unsuccessful growth preserves existing state.

These stages teach the same abstraction with increasing responsibility. They also show how a simple algorithm can remain unchanged when its storage policy changes.

## 12.11 Progressive text exercises

First write an ASCII decimal parser that rejects every non-digit. Next permit leading spaces while still requiring at least one digit. Then add an optional sign and specify the behavior for signed minimum and maximum values. Finally switch from a terminating string to a counted byte span.

For each stage, list the accepted grammar explicitly. Tests should include zero, negative values when allowed, empty input, trailing junk, maximum representable values, and one value beyond the chosen range.

## 12.12 A bounded data pipeline

Combine three earlier projects into an application: read a binary message, validate its header and checksum, then append a decoded value to a fixed-size queue. Your contract must explain what happens when the queue is full. Does the program reject the message, drop the oldest value, or report backpressure? Each is a meaningful policy choice.

The test should inject short packets, malformed lengths, checksum failures, and queue saturation, and verify that rejected packets do not alter queue contents.

## 12.13 A portability investigation

Compile a set of probes that report `CHAR_BIT`, `sizeof(int)`, `sizeof(void *)`, the availability of `uint32_t`, and support for selected C23 features. Do not treat the output of one workstation as universal. Repeat on a second compiler, a different architecture, or a freestanding target where possible.

Compare the results with the preconditions stated in the book's byte-oriented and dynamic-memory examples. This exercise joins language rules to engineering practice.

## 12.14 A review rubric

Assess work in four dimensions, not one. **Correctness**: do the algorithms meet their contracts? **Safety**: are arithmetic and storage bounds checked? **Clarity**: can another engineer understand the code and its assumptions? **Verification**: do the tests challenge boundaries rather than merely demonstrating one successful input?

A concise, correct implementation with explicit limits is preferable to a larger one that hides failures in complicated abstractions.

**Further problems:** Review a peer's parser without running it, then compare your findings with compiler and sanitizer output. Build a small executable that verifies a public header compiles independently. Explain which claims in a test report are observed and which remain assumptions.
