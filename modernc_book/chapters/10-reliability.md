# 10. Reliable Systems Programming

The language gives precise rules for certain operations and deliberately leaves other matters to implementations or gives no meaning to invalid programs. Reliability begins with knowing the boundary.

## 10.1 Undefined, unspecified, implementation-defined

An **undefined** operation has no required behavior: indexing past an array or signed overflow are examples. **Implementation-defined** behavior obliges an implementation to document a choice, such as properties of `char`. **Unspecified** behavior allows a choice among permitted outcomes without requiring that choice to be documented.

Never make the program's correctness depend on one observed result of undefined behavior, even if a particular build seems stable.

## 10.2 Check sizes before multiplying

```c
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

bool allocate_samples(size_t count, int **out)
{
    if (out == nullptr) return false;
    if (count == 0 || count > SIZE_MAX / sizeof(int)) return false;
    int *p = malloc(count * sizeof *p);
    if (p == nullptr) return false;
    *out = p;
    return true;
}
```

The caller must release `*out` after success. The function leaves `*out` unchanged on failure. For C23, `bool` is a keyword. For maximum compatibility with older dialects, include `<stdbool.h>`.

## 10.3 The limits of volatile

`volatile` is useful for accesses with externally observable behavior, including certain memory-mapped I/O, when used according to implementation and hardware rules. It is not a synchronization primitive and does not prevent data races. Atomics from `<stdatomic.h>` provide operations with explicit memory-order semantics, when supported by the target's implementation.

## 10.4 Host tests and device tests

Test numerical functions on a host where instrumentation is available. Enable sanitizers such as AddressSanitizer and UndefinedBehaviorSanitizer where toolchains support them. Unit tests do not replace target validation for memory maps, interrupts, scheduling latency, peripheral behavior, or floating-point differences.

## 10.5 Protocol parsing

A parser accepting a length and a byte buffer is easier to audit than one that searches indefinitely for a terminator. Distinguish malformed input, insufficient data, overflow, and unsupported versions. Parse into temporary state and publish a complete result only after validation.

## 10.6 Review checklist

Ask of each interface: Are sizes explicit? Are output objects initialized on success only? Is ownership documented? Are integer conversions checked? Are assumptions about endianness and alignment written down? Can cleanup happen on every error path? Is concurrent access synchronized?

**Exercises.** 10.1 Enumerate five undefined-behavior risks in a string decoder. 10.2 Design a bounded ring buffer and define its full and empty states. 10.3 Explain why `volatile int flag` is not generally a safe cross-thread signal.


## 10.7 Establish invariants before optimizing

An invariant is a property required to remain true across operations. A ring buffer may maintain `count <= capacity`; a vector may maintain `length <= capacity`; a parser may maintain `position <= input_length`. The easiest time to discover a broken invariant is at the operation that first violates it.

Checks should precede the risky operation. Checking a pointer after dereferencing it, or checking a sum after signed overflow, is too late. An optimizer may assume undefined behavior never occurs and transform subsequent code accordingly.

## 10.8 Bound both time and storage

An embedded function should be evaluated not only for logical correctness but also for its worst-case time and memory needs. A loop over a fixed-size buffer has an explicit bound. A dynamically grown table or a scan to an unbounded terminator may not.

Real-time behavior adds scheduling, interrupt latency, contention, and priority inversion. None can be solved merely by choosing C23 syntax. The language supports expressing the operations; the system architecture determines when they can run.

## 10.9 Concurrency and atomics

Sharing a non-atomic object between threads without appropriate synchronization can create a data race, resulting in undefined behavior. `volatile` does not repair this. Atomics in `<stdatomic.h>` provide carefully specified atomic operations; mutexes and other primitives may be preferable where a critical section covers several related objects.

Memory-order choices require a proof about which values must be visible to which threads. Start with sequentially consistent operations unless the design has a demonstrated reason to use weaker ordering. Interrupt-to-thread communication also depends on the implementation's interrupt model and target hardware.

## 10.10 Errors are part of the API

A function's result contract should specify what happens when it receives a null pointer, an invalid length, an unrecognized packet, insufficient output storage, or a resource failure. The failure behavior may need to preserve caller state. Document that guarantee and test it.

The absence of a crash does not prove memory safety. A parser can mis-handle a length while still writing inside allocated storage, and a numerical routine can return a plausible but incorrect value after an invalid conversion.

## 10.11 Exercises

**10.4** Write invariants for a fixed-capacity queue and construct tests preserving them. **10.5** Make a parser reject overflow before computing `offset + length`. **10.6** Describe what synchronization an interrupt-driven producer and thread consumer would require. **10.7** Identify whether each of four sample failures is undefined, unspecified, or implementation-defined behavior.


## 10.12 A bounded packet decoder

A practical parser should never trust an unvalidated input length. The companion program `examples/packet_decode.c` reads a miniature binary packet with magic byte `0xA5`, version byte `1`, a two-byte little-endian payload length, at most 64 payload bytes, and a one-byte XOR checksum. This deliberately simple checksum demonstrates framing but does **not** provide cryptographic integrity or meaningful protection against intentional tampering.

The parser first checks the minimum header and checksum size. Next it validates the magic and version. It constructs the declared payload length from bytes, without casting the input buffer to a structure. Before accessing the payload, it verifies the declared length against both the fixed maximum and the available input bytes. Only after validating the complete packet and checksum does it write the decoded result into the caller's object.

One subtle design choice is to build a local candidate object and assign it to the caller only on success. This gives failure an important contract: the output remains unchanged. The implementation uses the subtraction `size - HEADER_SIZE - 1` only after proving the total size is at least `HEADER_SIZE + 1`, so the subtraction cannot underflow.

The program also exercises an invalid checksum and truncated input. It was compiled and executed locally using GCC 14 in C2x dialect, with `-Wall -Wextra -Wpedantic -Werror` and AddressSanitizer and UndefinedBehaviorSanitizer enabled. This verifies that executable under the tested configuration, not every possible input or toolchain.

**Exercises.** 10.8 Add a failure reason enum without changing the output-on-failure guarantee. 10.9 Generate exhaustive byte-sequence inputs for packets of length zero through four. 10.10 Replace the XOR checksum with a documented CRC and explain why even a CRC is not a cryptographic authentication code.


## 10.13 Preconditions, invariants, and postconditions

A robust C function can be described by three kinds of statements. Preconditions specify what the caller must supply; invariants describe properties that remain true during execution; postconditions describe what success and failure mean.

For a byte decoder, a precondition may be that any non-null buffer pointer refers to at least the supplied length of readable storage. An invariant may be that the parser index never exceeds the input length. The successful postcondition is a fully decoded object; the failure postcondition is that the output object remains unchanged.

These distinctions matter because not every invalid pointer can be detected with a null check. A caller that lies about an object's bounds violates the contract. The function can still protect itself against internally computed out-of-range indexes by validating lengths before access.

## 10.14 A two-stage parser

An effective pattern is *validate, then commit*. Use temporary state to decode a candidate; inspect the entire record; only then write the result.

```c
struct header candidate = {};
/* Parse candidate from a counted byte buffer. */
if (!valid_magic || !valid_length || !valid_checksum)
    return false;
*out = candidate;
return true;
```

This is a fragment, not a standalone example. The important point is that errors do not leave a partially updated public result. Chapter 10's packet example uses a bounded payload and this publication policy.

## 10.15 Safety is not security

Memory safety protects against invalid storage access. A parser can be memory-safe yet accept maliciously large, inconsistent, or semantically forbidden requests. A one-byte XOR checksum may detect some accidental errors but is not authentication; an attacker can usually alter data and restore a valid XOR.

For an external protocol, consider resource exhaustion, replay, malformed framing, version downgrades, and authorization separately from buffer bounds. The task determines whether cryptographic integrity, rate limits, and stronger protocol validation are required.

## 10.16 Bounded execution requires a system model

Worst-case running time is more than an algorithmic big-O expression. In a real-time task, include interrupt preemption, cache behavior, memory contention, blocking calls, and other scheduling delays. A loop of exactly 128 iterations is bounded in iteration count; it is not automatically guaranteed to meet a microsecond deadline under every scheduler and cache state.

Avoid heap allocation and unbounded blocking on a critical path unless their timing has been proven acceptable. For many embedded applications, fixed-size data structures, preallocation, and explicit timeout behavior simplify the argument.

## 10.17 Reviews use independent evidence

Different validation methods find different errors. Compiler warnings detect suspicious constructs. Static analysis explores possible paths. Sanitizers catch classes of runtime faults when the relevant paths execute. Property tests explore behavioral invariants. Hardware-in-the-loop tests reveal device-specific assumptions.

No single passing test suite proves correctness, particularly for concurrent programs. For a critical algorithm, pair executable tests with a short written proof sketch: define ranges, explain why every memory access is within them, and explain why every loop terminates.

## 10.18 A fault-injection exercise

Build a streaming decoder with an injected byte source that can return a short read, an I/O error, or clean EOF. Give each outcome a distinct status and assert that the caller never sees half-decoded output. Repeat with a deliberately full output buffer and with a record whose claimed payload length is larger than available storage.

**Further problems:** Construct a threat model for the toy packet protocol. Explain how a full buffer and an overflowed integer differ. Write a reviewer checklist for atomic queue shutdown and object lifetime.


## 10.19 Defensive decoding: from bytes to a validated object

A packet decoder is a good place to practice several C contracts at once. A caller supplies a byte buffer and its length. The decoder must establish a valid format before touching output state.

For a toy protocol, define the frame as a one-byte magic number, one-byte version, two-byte little-endian payload length, payload bytes, and a checksum octet. The decoder first confirms that at least the fixed header and checksum are present. Only then does it read the header fields. Next it verifies the payload length against both its application-defined maximum and the remaining input capacity. Finally, it computes the checksum and copies the completed candidate into the caller's output.

The order matters. If code reads `bytes[3]` before checking `length >= 4`, a later bounds check cannot undo the invalid read. If it calculates `payload_length + 5` without checking representability, a large unsigned value could wrap. If it writes the public result field-by-field and later discovers an invalid checksum, the caller sees partially updated state.

A strong decoder therefore has a simple publication point: all validation succeeds, and only then is `*out` assigned.

## 10.20 Define three failure classes

Not all failures have the same meaning. A useful API might distinguish malformed input, incomplete input, and exhausted output capacity. Malformed input violates the format even if more bytes were supplied. Incomplete input may be a valid prefix that needs more data. Insufficient output capacity means the input might be valid, but the caller did not provide enough destination storage.

An RTOS message receiver can react differently: wait for the rest of an incomplete frame, discard malformed input and resynchronize, or apply backpressure when output storage is full. Collapsing all three into a boolean failure can make the transport logic awkward.

The simplest teaching program can return `bool`, but a production interface should choose the status representation based on its callers.

## 10.21 Error recovery is a protocol decision

After rejecting a malformed frame, should the receiver scan for the next magic byte? Doing so may recover quickly, but payload bytes could themselves contain that marker. A more robust framing design might escape reserved bytes, use a fixed header with a validated length and checksum, or rely on an out-of-band boundary.

The algorithm's *resynchronization* rule belongs in the protocol specification. Otherwise two devices may interpret the same corrupt stream differently despite both having memory-safe parsers.

## 10.22 Worst-case resource accounting

To evaluate firmware use, compute worst-case storage and work. A parser with a maximum 64-byte payload, a four-byte header, and a checksum can bound its input record storage. The actual C output structure may be larger due to padding, so assess `sizeof` on the target when reserving a pool of output records.

The per-record checksum loop has at most a fixed number of iterations. That proves an iteration bound, not a deadline: execution time also depends on scheduling, memory placement, compiler settings, and hardware. A task that blocks on a mutex or output stream may take longer than its local loop suggests.

### Exercises

**10.11** Implement a status enumeration with valid, incomplete, malformed and no-capacity cases. **10.12** Design a resynchronization policy when a payload contains the magic byte. **10.13** Calculate an upper bound on parser storage independent of input stream length. **10.14** Test that all error paths preserve the output structure's meaningful members.
