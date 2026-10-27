# 12. Exercises and Further Reading

The best way to learn C is to predict a program's behavior before compiling it, then explain any discrepancy. Exercises should include ordinary cases, boundaries, and failure cases.

## 12.1 A progression of projects

**Project A — Bounded line reader.** Implement a function that fills a caller-supplied buffer, reports bytes stored, distinguishes newline from EOF, and signals truncation. Write tests for zero-capacity buffers, one-character buffers, long lines, embedded zero bytes, and input errors.

**Project B — Integer parser.** Parse unsigned decimal text into `unsigned` without `atoi`. Reject empty input and nondigits; detect overflow before multiplying by ten or adding the next digit. Show why checking after an arithmetic overflow is too late.

**Project C — A fixed pool.** Implement allocation and release of equal-sized records from a fixed array. Reject invalid and repeated releases. Describe whether the pool can be used concurrently, and if so how it is synchronized.

**Project D — Binary message reader.** Decode a header with a version byte, 16-bit little-endian payload length, and checksum. Check all bounds before decoding. Test malformed lengths and truncated buffers.

**Project E — Word index.** Improve Chapter 11 with a hash table, sorting, and a test driver. Benchmark it using increasing vocabulary sizes and measure memory usage.

## 12.2 How to study source code

Read short functions and annotate their contracts. What objects must exist at entry? What inputs are valid? Which values may overflow? What state changes after success? What state is preserved after failure? Which implementation-defined assumptions are present?

Compiler warnings are evidence to inspect, not text to suppress reflexively. Sanitizers find bugs only on executed paths; code review and carefully constructed tests remain essential.

## 12.3 References

- ISO/IEC 9899:2024, *Programming languages — C* (normative specification).
- cppreference, C language reference, especially its C23 features and library pages.
- GCC C standards implementation status and command-line dialect options.
- Clang C language status and diagnostics documentation.
- The ISO C Secure Coding Rules and CERT C guidance, for additional defensive-programming perspectives.

## 12.4 A note on editions

This is an independent original textbook. Classic C teaching remains valuable for its economy of examples, but 2026 readers also need accurate discussion of defined behavior, lifetime, ownership, toolchain variation, safer interfaces, and C23's additions. Future manuscript revisions should add tested solutions, further real-world worked examples, portable build checks, and an index before a complete book-length edition is declared.

# 12.1 Exercises as specifications

An exercise is complete only when it states what the program accepts, what it returns, and how failure is reported. Consider a decimal parser: what does it do with an empty string, leading spaces, a plus sign, invalid characters, and a number larger than the destination type? The interesting part of the problem is often deciding these details before coding.

## 12.5 Testing boundaries

For a bounded collection with capacity N, test insertion at sizes zero, one, N−1, and N, then one additional insertion that must fail without damaging existing records. For signed arithmetic, test the largest and smallest representable values, opposite signs, and zero. For a byte parser, test all shorter-than-required inputs.

## 12.6 Integration project: a streaming index

Read Chapter 11's sorted word index and complete the following experiments:

1. Validate a file containing zero words and a file ending with an unterminated word.
2. Check that capitalization is folded only for ASCII letters and that digits remain in the word.
3. Generate 1,024 unique words and verify the table fills without overrun.
4. Attempt one more unique word and confirm the operation fails.
5. Compare counts against an independent reference implementation for randomly generated ASCII input.
6. Measure runtime for sorted and reverse-sorted unique vocabularies to investigate insertion cost.
7. Replace the sorted array with a fixed-capacity open-addressed hash table; state the collision policy, load factor, and failure semantics.
8. Describe how a UTF-8 version would need a more sophisticated definition of words, code points, and normalization.

## 12.7 Integration project: a bounded vector

Start from `examples/vector.c`. Add `reserve`, `clear`, and `get` operations with contracts. Make failed growth preserve the old allocation and data. Test failed allocation by injecting an allocator that rejects selected requests. Discuss why assigning one vector object to another is not a deep copy.

## 12.8 Reading C with a reviewer

For every function, identify which variables represent length and capacity, which arithmetic might overflow, which objects are borrowed versus owned, whether outputs change on failure, which operations may allocate, and whether concurrent calls can safely share state. A program may be short and still require serious reasoning.

## 12.9 Challenge: a portable protocol reader

Design a packet decoder accepting `const unsigned char *bytes` and `size_t length`. A packet contains a magic byte, a version, a two-byte little-endian payload length, payload bytes, and an eight-bit checksum. Reject incomplete packets before reading fields; reject lengths inconsistent with the buffer; do not write the output structure until the checksum passes. Provide cases for exact minimum length, maximum supported payload, bad version, invalid checksum, and length overflow.

## 12.10 Challenge: concurrency

Design a producer-consumer queue for one interrupt-side producer and one thread-side consumer. State why Chapter 5's plain queue is not automatically safe under concurrent access. Choose either an RTOS synchronization primitive with appropriate interrupt semantics or a target-validated atomic design. Document memory-order assumptions and why the chosen operations are legal in both execution contexts.

## 12.11 Further reading in practice

After solving a problem, read the relevant normative language clause or trusted reference and compare it with the implementation's documentation. A standards-conforming program is not merely one that a compiler accepts; conformance involves the behavior required by the language and library specification.


## 12.12 How to write convincing solutions

A solution should include more than a working program. State assumptions, list invariants, justify any bounds arithmetic, and show representative tests. A design that accepts invalid input only because one particular compiler tolerated it is not a complete solution.

A useful submission format has five short parts: (1) contract, (2) algorithm, (3) implementation, (4) test table, and (5) limitations. In a small exercise this may fit on one page. In a hardware-related problem, also identify dependencies on the toolchain and target.

## 12.13 Progressive memory exercises

Start with a fixed array of integers. Add a function returning its count of nonzero elements. Next write a version accepting a pointer and a length. Then add dynamic allocation with a checked size calculation. Finally add a failure-injecting allocator and verify that unsuccessful growth preserves existing state.

These stages teach the same abstraction with increasing responsibility. They also show how a simple algorithm can remain unchanged when its storage policy changes.

## 12.14 Progressive text exercises

First write an ASCII decimal parser that rejects every non-digit. Next permit leading spaces while still requiring at least one digit. Then add an optional sign and specify the behavior for signed minimum and maximum values. Finally switch from a terminating string to a counted byte span.

For each stage, list the accepted grammar explicitly. Tests should include zero, negative values when allowed, empty input, trailing junk, maximum representable values, and one value beyond the chosen range.

## 12.15 A bounded data pipeline

Combine three earlier projects into an application: read a binary message, validate its header and checksum, then append a decoded value to a fixed-size queue. Your contract must explain what happens when the queue is full. Does the program reject the message, drop the oldest value, or report backpressure? Each is a meaningful policy choice.

The test should inject short packets, malformed lengths, checksum failures, and queue saturation, and verify that rejected packets do not alter queue contents.

## 12.16 A portability investigation

Compile a set of probes that report `CHAR_BIT`, `sizeof(int)`, `sizeof(void *)`, the availability of `uint32_t`, and support for selected C23 features. Do not treat the output of one workstation as universal. Repeat on a second compiler, a different architecture, or a freestanding target where possible.

Compare the results with the preconditions stated in the book's byte-oriented and dynamic-memory examples. This exercise joins language rules to engineering practice.

## 12.17 A review rubric

Assess work in four dimensions, not one. **Correctness**: do the algorithms meet their contracts? **Safety**: are arithmetic and storage bounds checked? **Clarity**: can another engineer understand the code and its assumptions? **Verification**: do the tests challenge boundaries rather than merely demonstrating one successful input?

A concise, correct implementation with explicit limits is preferable to a larger one that hides failures in complicated abstractions.

**Further problems:** Review a peer's parser without running it, then compare your findings with compiler and sanitizer output. Build a small executable that verifies a public header compiles independently. Explain which claims in a test report are observed and which remain assumptions.


## 12.18 Guided project: a bounded arena

A fixed byte arena is a compact way to practice both arithmetic and ownership. The repository's `examples/arena_bytes.c` contains a reference implementation. Before reading it, write your own design specification.

Your arena contains an array of 128 `unsigned char` elements and a `size_t used` offset. It must accept a request for a nonzero number of bytes if the request fits in the unallocated suffix. A successful request returns a pointer to that region and advances the offset. A failed request must leave the offset and the caller's output pointer unchanged.

Work through three stages.

**Stage A: capacity proof.** Write the invariant `used <= capacity`. Show that after checking it, the expression `capacity - used` is defined and nonnegative in the mathematical sense. Check `count <= capacity - used` before adding count to the offset.

**Stage B: behavior.** Allocate five bytes, then seven bytes. Confirm that the second returned pointer lies immediately after the first region. Request more than the remaining capacity and ensure failure changes no state. Fill the arena exactly and verify that the next one-byte request fails.

**Stage C: reuse.** Reset the arena offset. Confirm a fresh request starts at the first byte. Explain why every previously borrowed region must now be treated as invalid under the arena's logical lifetime contract.

This project allocates byte spans, not arbitrary typed objects. To extend it into a general-purpose typed arena, investigate alignment, backing-storage guarantees, object access rules and target DMA constraints before introducing casts.

## 12.19 Guided project: signed decimal input

Write a parser for `long` with an optional sign followed by one or more ASCII decimal digits. Reject spaces and trailing garbage. Failure must leave the output unchanged.

The challenge is `LONG_MIN`. On a two's-complement implementation it has a magnitude one greater than `LONG_MAX`. If you accumulate a positive signed magnitude, the parser cannot reach the most negative legal result without first trying to represent an illegal intermediate value.

Accumulate an unsigned magnitude with a sign-dependent limit. Before each multiplication by ten, verify that the new value will fit. When the final input is the exact negative endpoint, assign `LONG_MIN` directly. Use `snprintf` to produce boundary strings on the actual implementation, rather than hardcoding an assumed `long` width.

A good test sequence includes empty input, a bare plus or minus sign, zero, negative zero, leading zeros, ordinary positive and negative numbers, `LONG_MAX`, `LONG_MIN`, and strings whose magnitudes exceed both endpoints.

Compare the final solution with `examples/parse_signed.c`. Then adapt the interface to accept a counted byte buffer so it does not require null termination.

## 12.20 Guided project: portable rotation

Implement a left and right 32-bit rotation. Define rotation counts modulo 32; this means counts 0, 32, and 64 leave the value unchanged.

Your implementation must never evaluate a shift whose count is 32. Handle the normalized zero case separately. Prove that for all other counts, both shift quantities are between one and 31 inclusive.

Test against independent hexadecimal constants. Then test the inverse property that rotating left and right by the same count restores the original value. This property is useful but insufficient by itself; two incorrect functions could still cancel one another.

Finally, explain why a target CPU's presence or absence of a rotate instruction does not change the C source's required semantics. Optional assembly belongs behind a target-specific interface and must be justified by measurement.

## 12.21 Guided project: ownership and aliases

Draw the memory objects involved in a vector whose structure contains pointer, length and capacity. Add three integers through its append operation, then create a second structure by copying the first.

Identify which fields have independent storage and which pointer values designate the same heap allocation. Does modifying an element through one structure affect the other? What happens if the first vector is destroyed? Can the second now be destroyed safely?

Next, create a borrowed pointer to the first element and append enough values to trigger a successful reallocation. Explain why that old pointer is invalid even if the allocator returns an address with the same numerical value.

Finally, add an injected allocation failure. Confirm that a correctly implemented append leaves every existing element and ownership field unchanged. This is an example of a strong failure guarantee, not an attempt to make shallow copies safe.

## 12.22 Review a protocol without compiling it

Given a packet consisting of a magic octet, version, two length octets, payload, and checksum, write a code-review checklist before implementing the parser. At what point may each byte be read? How is the length decoded? When can a subtraction underflow? What is the maximum payload? What happens if extra trailing data exists? When is output state published?

Now implement the decoder, write manual golden test vectors, and run it with sanitizer instrumentation if available. Keep the input-buffer contract explicit: a non-null pointer and a numeric length do not by themselves prove that storage of that size exists.

This exercise combines contracts from Chapters 4, 7, 10 and 14. Its outcome should include an implementation, a test matrix and a short reasoning argument.

## 12.23 Writing up the result

For each project, include a short report: the precise accepted input domain, key invariants, failure semantics, tests, compiler mode, and known limitations. Differentiate ordinary input errors from caller contract violations and from implementation-specific assumptions.

A particularly useful final section is *what the tests do not prove*. A host byte-arena test does not establish general typed-object alignment. A sequential queue test does not prove concurrent safety. A passing frame-decoder test does not authenticate a message. A clean sanitizer run cannot prove that unexecuted paths are safe.

That level of precision distinguishes a professional systems-programming exercise from a demonstration that happened to run once.

## 12.24 Capstone: a bounded telemetry receiver

This project combines several of the book's smaller components into one coherent embedded-style design. The goal is to receive short binary frames, validate their content, and retain a bounded number of decoded messages for a consumer.

The receiver has three layers:

    incoming octets -> frame parser -> fixed slot pool -> application consumer

The first layer supplies octets. The second recognizes complete, valid frames. The third stores completed messages until the application consumes them. Each layer has a different contract, and no layer needs to allocate memory dynamically.

A finished implementation should make all failure paths visible: malformed input, an incomplete frame, exhausted message slots, a stale handle, and shutdown while a message is still outstanding.

### Part 1: define the external format

Use the same compact frame as Chapter 10: magic A5, a length octet between zero and eight, the declared number of payload octets, and an XOR checksum covering the length and payload.

Create a table containing at least these independent test frames:

| Meaning | Hexadecimal frame |
| --- | --- |
| Empty message | A5 00 00 |
| One octet 05 | A5 01 05 04 |
| Three octets 0A 14 1E | A5 03 0A 14 1E 03 |
| Invalid length | A5 09 |
| Incorrect checksum | A5 01 05 00 |

For the last two entries, a complete application test must check that no new message is published. The invalid-length example represents a rejected prefix; it does not need a valid checksum because length validation fails first.

Explain why successful encoding and decoding in a loop is not enough to verify the format. The independent table protects against two implementations that share the same mistake.

### Part 2: identify the state owners

Create one frame parser and a pool containing four fixed message records. Make a single receiver task the owner of the parser. The receiver calls the feed function for each incoming octet.

When a complete message appears, the receiver attempts to acquire a free pool slot. If acquisition succeeds, it copies the completed payload into the slot and publishes a handle to the application consumer. If the pool is full, the receiver must report or count a dropped message according to the chosen policy.

The application consumer eventually retrieves a message using the validated handle and releases its slot. After release, the old handle is no longer valid.

This arrangement separates the lifetime of a temporary candidate message within the parser from the lifetime of a completed message retained by the pool.

### Part 3: choose a queue and saturation policy

A real producer and consumer often run at different rates. A pool alone stores objects but does not define the order in which completed-message handles are delivered.

Add a fixed-capacity queue of handles. When the receiver decodes a message successfully, it acquires a slot, stores the message, and enqueues the handle. The consumer dequeues a handle, processes the corresponding message, and releases the slot.

Specify what happens if either the pool or queue is full. Possible choices include rejecting the newest message, dropping the oldest queued message, or applying backpressure to the transport. Dropping an old message requires care: remove its handle from the queue and release its pool slot exactly once.

Do not silently overwrite a live slot. A bounded memory design is useful only when saturation has predictable semantics.

### Part 4: distinguish host tests from device behavior

On a desktop, a test can supply input bytes from a fixed array and run receiver and consumer operations sequentially. Verify the exact decoded data, queue order, handle generations, full conditions, and stale-handle rejections.

That test establishes the algorithm's expected behavior for the chosen sequence. It does not demonstrate that concurrent producer and consumer access is safe.

If the project uses an RTOS and UART interrupts, select or implement a queue whose operations are supported in the relevant execution contexts. Establish how the ISR hands data to a receiver task and how that task signals the consumer. Avoid having the ISR call an arbitrary C function that may block or use unsupported library facilities.

### Part 5: bound resources

Document the maximum storage required by:
- the frame parser's candidate payload;
- all four message-pool records;
- a fixed queue of message handles;
- any transport byte buffer;
- the stacks of the participating tasks.

These are separate allocations or objects, and their sizes may include alignment padding. Confirm their actual sizes on the target toolchain using its build output or suitable instrumentation.

Next, bound local operations. One call to the frame parser processes one octet; a pool acquire examines at most four slots; a queue enqueue performs a fixed amount of index work under its stated conditions.

This gives useful operation bounds but not a complete worst-case deadline analysis. Real-time completion also depends on preemption, queue contention, memory access times, and the scheduling of the consumer.

### Part 6: design shutdown

Shutting down the receiver requires a policy for incomplete frames and pending messages. One possible sequence is to stop new input, allow the receiver to finish processing already queued octets, signal the consumer, drain or deliberately discard the remaining messages, join or stop all participating tasks, and only then reclaim storage.

If a queued handle is discarded, release its pool slot. If a consumer still holds a handle, the pool cannot be reset or destroyed until that use is finished under the selected ownership contract.

Shutdown bugs often arise because ordinary tests exercise steady-state operation but never release resources while work remains.

### Required test report

Submit a short report describing the accepted frame grammar, all data-structure invariants, capacity and drop policy, lifetime of every borrowed pointer and handle, executable tests, target-specific assumptions, and remaining unverified properties.

A particularly useful test combines several failures: begin a partial frame, abandon it on timeout, fill the pool with valid messages, reject an additional completed frame, release one slot, then accept a new message into that slot. Confirm that an old handle does not refer to the new record.

A clear architecture with documented limits is more important than using the most complicated available synchronization technique.

## 12.25 A short review rubric for the capstone

A high-quality submission should demonstrate correctness at four levels:

**Representation.** Each binary field is decoded according to the stated wire format, with no dependence on native structure padding or endianness.

**State.** Parser transitions, pool occupancy, handle generations, and queue indexes preserve their written invariants.

**Ownership.** Every acquired slot is released once, stale handles are rejected, and shutdown does not destroy storage that active clients still use.

**Execution.** Tests cover valid boundaries, truncation, corruption, saturation, and recovery. Concurrency and hardware-related claims are qualified by appropriate target-specific evidence.

The goal is not to build a production-ready communications stack in one exercise. It is to practice decomposing a small real system into contracts that can be reasoned about and tested independently.
