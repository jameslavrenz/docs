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


## 10.23 A bounded binary record format

A type-length-value (TLV) record stores each field as a type identifier, a length, and a sequence of value bytes. This pattern appears in binary protocols, embedded device configuration, and extensible message formats. The exact field widths and constraints must be specified; the letters 'TLV' alone do not define a universal byte layout.

For our teaching protocol, each field has:

- One octet identifying the tag.
- One octet containing an unsigned byte count.
- Exactly that many following payload octets.

A complete message is a concatenation of zero or more fields. The application imposes two further limits: at most four fields and at most eight payload octets per field. Repeated tags are permitted, and a zero-length field is valid.

Those limits bound memory independent of the input's advertised lengths. The parser rejects a message that exceeds them rather than allocating more storage or truncating a field.

The complete implementation is `examples/tlv_decode.c`.

## 10.24 Parsing is a sequence of narrowing proofs

A byte array supplied by the caller does not have the same type as the application record. The decoder must establish that every external field fits the chosen internal representation.

Suppose `cursor` is the number of input bytes already consumed and `size` is the supplied input length. Maintain the invariant:

```text
0 <= cursor <= size
0 <= decoded_field_count <= 4
```

Before reading a two-octet field header, test `size - cursor >= 2`. That subtraction is safe because the cursor invariant already holds.

Read tag and length only after this test. Advance cursor by two, then check that the length is no greater than eight and that `length <= size - cursor`. Only after these checks is it valid to copy the payload bytes from the source.

This approach avoids forming potentially overflowing sums such as `cursor + header_size + length` before checking bounds. It also avoids reading the length octet from a truncated one-byte header.

## 10.25 A strong failure guarantee

A decoder that fills its caller's result structure incrementally can leave partially decoded data on a malformed input. That is not always forbidden, but it complicates callers that want a previous valid configuration to survive a failed update.

The TLV decoder instead begins with a local empty candidate. It appends each validated field to that candidate and assigns the whole result to `*out` only after consuming the complete input successfully.

```c
struct tlv_message candidate = {};
/* Validate every field and fill candidate. */
*out = candidate;    /* Exactly one publication point. */
return true;
```

The program intentionally retains a simple `bool` result: malformed input, excessive field count, and oversized field values all return false. For a production protocol, an enumeration distinguishing those failure causes may be more useful.

On failure, no meaningful member of the output result is modified. The implementation tests this guarantee after a valid two-field message by attempting several malformed messages and checking that the earlier result is retained.

## 10.26 What zero-length input means

The grammar permits an empty message containing zero TLV fields. Consequently, the decoder accepts `bytes == nullptr` when `size == 0` and produces an empty result. It rejects a null input pointer when the supplied size is nonzero.

This explicit convention avoids a common C pointer mistake: attempting to construct an end pointer by writing `nullptr + 0`. The decoder uses indexes and never performs pointer arithmetic on a null input.

An application might instead require at least one field and reject an empty message. That is a valid alternative, but the rule belongs in its specification rather than being implied by an implementation accident.

## 10.27 Truncation and extra bytes

Consider these byte sequences:

```text
01 03 0A 0B 0C   Valid field: tag 1, three bytes 0A 0B 0C
01 04 0A 0B 0C   Invalid: claims four bytes, supplies three
01               Invalid: incomplete field header
01 00 02 00      Valid: two empty fields, tags 1 and 2
```

The decoder does not accept the valid prefix of a malformed message and ignore trailing or missing bytes. Its loop must consume exactly the supplied length in complete TLV fields.

The tests also supply a field whose claimed length exceeds eight and a message containing five empty fields. Both are rejected by application limits even though the one-byte length value itself is representable.

Memory safety is necessary but not sufficient for protocol correctness. A decoder could remain within every buffer while silently ignoring an unsupported tag or interpreting an unknown field incorrectly. A real application often adds tag validation, mandatory-field checks, versioning, and field-order rules.

## 10.28 Unknown fields and forward compatibility

A flexible protocol may allow unknown tag values to be skipped using their lengths. This can help newer senders communicate with older receivers. But skipping a field is safe only after its claimed length has been validated against the available input bytes.

Different tags may also have different allowed lengths. A timestamp might require exactly four octets, while a device name might be a variable-length text field. A general decoder can first validate the outer TLV shape and then run a second semantic validation stage for each recognized tag.

Do not assume unknown fields are harmless. A security- or safety-critical field marked mandatory might need to cause rejection when the receiver cannot interpret it. Protocol evolution requires an explicit compatibility policy.

## 10.29 Storage and timing bounds

The example's result structure reserves four field slots, each containing up to eight value octets plus tag and length metadata. Its maximum storage is known from the C type, although padding means `sizeof(struct tlv_message)` need not equal a simple sum of wire-format octets.

The decoder's loop can accept at most four fields before reaching its success condition. It rejects an attempted fifth field. Its payload copies are limited to eight octets each, establishing a straightforward bound on local work for valid and rejected records.

This is a useful design for small embedded messages: resource exhaustion becomes an ordinary error value rather than unexpected heap allocation or unbounded parsing.

However, if the surrounding transport waits indefinitely for a complete message, the application's end-to-end latency is not bounded merely because the parser is. Transport and parser timing must be assessed separately.

## 10.30 From parser to authenticated command

The educational TLV example intentionally has no checksum, authentication, anti-replay protection, or authorization. It must not be treated as a secure command protocol simply because it performs strict bounds checking.

A production command receiver should distinguish the integrity of transmitted data, the identity or authorization of the sender, and the semantic validity of the command. A CRC may detect accidental corruption; cryptographic authentication may be required when malicious modification is in scope. Bounds checks cannot provide those properties.

The separation is valuable architecturally. A framing layer obtains a complete message, an integrity layer verifies the appropriate check, the TLV decoder constructs a typed candidate, and the application verifies allowed operations before committing state.

### Additional exercises

**10.15** Modify the TLV decoder to return distinct statuses for truncated header, truncated payload, excessive field count, and oversized value. **10.16** Specify mandatory tags and reject duplicate instances of those tags. **10.17** Add an unknown-field skipping policy while preserving all bounds checks. **10.18** Build an independent expected-value test for two TLV fields rather than relying on an encoder written with the same algorithm. **10.19** Explain why a bounded parser is not automatically a secure or real-time protocol implementation.


## 10.31 Framing one byte at a time

The complete TLV decoder earlier in the chapter assumes that its caller already has a complete candidate message. Real serial links often provide only a few bytes at a time. A UART receiver might produce one octet per interrupt; a DMA operation might deposit a block; a network API may return a fragment that ends in the middle of a field.

A streaming frame parser makes progress incrementally. It keeps a small state object between calls and processes one received octet at a time. It never needs to allocate a buffer proportional to an untrusted advertised length.

For an educational frame format, define these octets:

```text
+--------+--------+----------------------+----------+
| magic  | length | payload (0..8 octets) | checksum |
+--------+--------+----------------------+----------+
  0xA5    0..8     exactly length bytes    XOR below
```

The checksum is the bitwise XOR of the length octet and all payload octets. The magic octet is not included. The format is intentionally simple enough to hand-calculate expected frames. It is not cryptographically secure, nor is its XOR checksum a robust choice for all error-detection environments.

The implementation appears in `examples/frame_stream.c`. It requires eight-bit octets and makes that assumption explicit at compile time.

## 10.32 Four parser states

The parser recognizes a frame using four states:

```text
             magic A5
EXPECT_MAGIC ----------> EXPECT_LENGTH
     ^                         |
     |                         | length 0
     |                         v
     |                    EXPECT_CHECK
     |                         ^
     |                         | last payload byte
     |                         |
     |                      EXPECT_PAYLOAD
     |                         ^
     |                         |
     +-------------------------+
       complete frame or error
       resets to EXPECT_MAGIC
```

The actual transitions are:

- `EXPECT_MAGIC`: accept only the framing marker and move to length; other octets are rejected.
- `EXPECT_LENGTH`: reject a value above eight; otherwise store the expected payload count and choose `EXPECT_CHECK` for zero-length payload or `EXPECT_PAYLOAD` for a nonempty payload.
- `EXPECT_PAYLOAD`: append exactly the announced number of octets, updating the running XOR.
- `EXPECT_CHECK`: compare the next octet with the computed XOR and either publish one complete frame or reject it.

This explicit state design is useful for RTOS tasks and event-driven firmware. Each call performs a bounded amount of work, independent of how long the input stream runs.

It does not guarantee that the task receives input at a bounded interval; scheduling, interrupt latency, and transport backpressure are separate requirements.

## 10.33 Invariants of the parser state

A helpful proof begins by identifying what each state makes true.

In `EXPECT_MAGIC`, no partial frame remains. The parser can accept a new framing marker.

In `EXPECT_LENGTH`, the magic octet has been accepted but the length has not yet been read.

In `EXPECT_PAYLOAD`, the declared length is between one and eight, and `received < length` before appending the next octet. Since the destination array has eight elements, the access `candidate.bytes[received]` is within bounds.

In `EXPECT_CHECK`, the announced payload length has been fully received, including the zero-length special case. The running checksum corresponds to the length and every received payload octet.

Every rejected frame resets the parser to its initial state. On success, the parser assigns its completed candidate to the output and then resets. That single publication point ensures callers never see a partly decoded public message.

In the example, the output is a `struct message` containing a count and an eight-element array. The parser owns a separate candidate structure and only copies it once validation has succeeded.

## 10.34 Bounded work per byte

The parser performs at most a few comparisons, one array store in the payload state, and an XOR operation for each input byte. It never calls `malloc`, loops over an untrusted count, or copies a variable-length region in one invocation.

Its memory requirement is constant: a parser state object and an output message supplied by the caller.

This establishes a useful algorithmic property: processing any one octet requires a bounded number of C operations. It does **not** establish a precise worst-case execution time on every MCU. Compiler optimization, interrupt preemption, cache behavior, memory wait states, and synchronization can affect actual latency.

When moving the parser to an RTOS, a good design separates the interrupt-level byte acquisition from higher-level frame interpretation. An ISR may deposit octets into a bounded queue; an ordinary task can then call `frame_feed`. Queue overflow and input dropping must have a documented policy.

## 10.35 An explicit three-result interface

The feed function returns one of:

| Result | Meaning | Public message |
| --- | --- | --- |
| `FEED_MORE` | A possible frame is incomplete | Unchanged |
| `FEED_COMPLETE` | A complete valid frame has been received | Updated |
| `FEED_REJECTED` | Invalid marker, length, checksum, or API state | Unchanged |

The caller can process completed messages without needing to inspect a partial buffer. It can ignore or count rejected octets according to its transport policy.

An error return does not necessarily mean an exceptional software fault. In a noisy communications link, malformed input is normal data that a decoder must handle safely.

The parser treats `FEED_MORE` as a legitimate result. A caller must not interpret it as a failure and reset the parser after every input fragment, or multibyte frames could never complete.

## 10.36 Checksum validation and publication

A frame with payload `0A 14 1E` and length three has a checksum obtained by XORing `03 0A 14 1E`:

```text
A5 03 0A 14 1E 03
|  |  |  |  |  +-- XOR checksum
|  |  +--+--+----- three payload octets
|  +-------------- length 3
+----------------- magic
```

The checksum happens to be `03` for this example. That coincidence is not part of the general format; different payloads produce different XOR results.

A failed checksum must not modify the caller's previously completed message. The parser resets its internal candidate and consumes the failing checksum octet.

The code checks this property by setting a recognizable output length, sending a corrupted frame, and verifying that the old output remains intact.

## 10.37 The subtle problem of resynchronization

The educational parser uses the simple policy **consume the rejected octet and reset**. It does not reconsider that octet as a possible magic marker.

This has a consequence. If the bad checksum octet equals `A5` and simultaneously would have been the start of a new frame, that potential new frame start is lost. Likewise, a magic octet inside a payload is treated as payload, not as a new frame boundary.

Neither behavior is necessarily a bug; it follows from the specified framing policy. However, a robust protocol must decide how receivers recover from corruption and where true message boundaries can be discovered.

A stronger design might use byte stuffing or escaping, a delimiter that cannot appear unescaped inside a payload, an authenticated record layer, or a scanning strategy with carefully tested recovery rules.

Searching blindly for the next `A5` is not automatically correct, because payload data may naturally contain that octet. The format, not just the parser code, determines what resynchronization can guarantee.

## 10.38 Timeout, truncation, and reset

An input source can stop providing bytes midway through a frame. For a parser at its initial state, clean stream termination means no incomplete frame remains. For a parser in any other state, final termination means the last frame is truncated.

The caller can query whether the parser is at a clean boundary and, on truncation, record an incomplete-input error. In an ongoing UART session, a temporary pause may not justify resetting the parser immediately. A higher-level timeout policy determines how long partial frames are retained.

The example exposes a reset operation for explicitly abandoning a partial frame. Reset is an ownership and protocol action: any bytes previously accumulated are discarded and cannot be recovered.

For applications requiring deadlines, a timestamp of the most recent received byte or an expected completion deadline can accompany the parser state. Timekeeping itself should remain outside the pure byte-decoding operation so host tests can supply deterministic timestamps.

## 10.39 How to test the state machine

Use independently calculated packets rather than relying exclusively on an encoder written by the same developer. The test suite covers:

- One valid frame containing three payload octets.
- A valid zero-length payload.
- An invalid advertised length above eight.
- A bad checksum that must not publish a result.
- A valid prefix that ends before all payload bytes arrive.
- Explicit reset after abandoning a truncated frame.

Additional recommended tests include every payload length from zero to eight, all possible single-byte corruptions of representative packets, and splitting a valid frame at each transport boundary.

A property-based test can generate valid frames and feed them one octet at a time. The resulting message must match the generated payload. A separate set of manually constructed vectors remains important because both a generator and decoder can share a format mistake.

## 10.40 Choosing a protocol interface

A whole-record parser is often simpler when the input transport supplies bounded complete messages. An incremental state machine is useful when records arrive in fragments or the application has a small per-invocation work budget.

Neither approach universally replaces the other. A stream parser often delegates completed frames to a whole-record semantic decoder. For example, `frame_feed` could identify and validate a small payload, then a TLV decoder could interpret fields inside that payload.

Each layer would have a distinct contract:
1. Transport supplies octets and reports delivery errors.
2. Framing recognizes one complete message.
3. Content decoding validates field representation and lengths.
4. Application logic checks the meaning, authorization, and allowed state changes.

That separation makes it easier to test algorithms on a desktop and replace only the transport adapter when moving to embedded hardware.

### Exercises

**10.20** Construct the byte sequence for an empty frame and explain every state transition. **10.21** Prove that the payload store can never write beyond index seven. **10.22** Design a separate timeout policy for an incomplete frame. **10.23** Explain what can happen when a failing checksum octet is also a magic marker. **10.24** Extend the parser with error categories for invalid marker, invalid length, and checksum failure. **10.25** Describe how an RTOS task would feed bytes acquired through a UART interrupt-driven ring buffer.
