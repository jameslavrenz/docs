# Appendix C. Selected Worked Exercises

These solutions illustrate contracts and tests rather than prescribe one clever implementation.

## C.1 Absolute difference without unsigned underflow

```c
unsigned difference(unsigned a, unsigned b)
{
    return a >= b ? a - b : b - a;
}
```

Both subtractions occur only in nonnegative order. Test `(0, 0)`, `(1, 0)`, `(0, UINT_MAX)`, and both operand orders.

## C.2 Checked parsing of an unsigned decimal

```c
#include <stdbool.h>
#include <limits.h>

bool parse_unsigned(const char *text, unsigned *out)
{
    if (text == nullptr || out == nullptr || *text == '\0')
        return false;

    unsigned value = 0;
    for (const unsigned char *p = (const unsigned char *)text;
         *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;
        unsigned digit = (unsigned)(*p - '0');
        if (value > (UINT_MAX - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    *out = value;
    return true;
}
```

The key inequality is evaluated *before* a potentially overflowing multiplication. The output is unchanged on failure. This version accepts no leading sign or whitespace, by design.

## C.3 Iterative factorial with an overflow check

```c
#include <stdbool.h>
#include <limits.h>

bool factorial(unsigned n, unsigned long *out)
{
    if (out == nullptr) return false;
    unsigned long result = 1;
    unsigned i = 2;
    while (i <= n) {
        if (result > ULONG_MAX / i) return false;
        result *= i;
        if (i == n) break;
        ++i;
    }
    *out = result;
    return true;
}
```

The explicit final-iteration break also avoids unsigned loop-counter wraparound if `n == UINT_MAX`. Checking multiplication alone is not the whole correctness proof.

## C.4 Little-endian byte decoding

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool read_u32_le(const unsigned char *bytes, size_t count, uint32_t *out)
{
    if (bytes == nullptr || out == nullptr || count < 4) return false;
    uint32_t result = (uint32_t)bytes[0]
                    | ((uint32_t)bytes[1] << 8)
                    | ((uint32_t)bytes[2] << 16)
                    | ((uint32_t)bytes[3] << 24);
    *out = result;
    return true;
}
```

The cast happens before shifting, so promotions do not make the top byte shift a signed overflow. Endianness is described by the wire format, not assumed from host memory.

## C.5 Cleanup on every path

```c
#include <stdio.h>
#include <stdbool.h>

bool write_record(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (fp == nullptr) return false;
    bool ok = fputs("record\n", fp) != EOF;
    if (fclose(fp) == EOF) ok = false;
    return ok;
}
```

Notice that `fclose` runs even after `fputs` fails. The function reports stream-level success, not guaranteed durable storage on physical media.


## C.6 A safe interval check

An interval starting at `offset` and containing `count` bytes fits within `capacity` when the start is valid and the count fits the remaining space:

```c
#include <stddef.h>
#include <stdbool.h>

bool fits(size_t offset, size_t count, size_t capacity)
{
    return offset <= capacity && count <= capacity - offset;
}
```

The short-circuit operator prevents unsigned subtraction from being evaluated if `offset` already exceeds capacity. Test zero capacity, exact fit, one-byte overrun, and `SIZE_MAX` boundaries.

## C.7 Checked allocation dimensions

```c
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

bool allocate_ints(size_t count, int **out)
{
    if (out == nullptr || count == 0 ||
        count > SIZE_MAX / sizeof(int))
        return false;
    int *p = malloc(count * sizeof *p);
    if (p == nullptr)
        return false;
    *out = p;
    return true;
}
```

The function rejects zero-sized requests and leaves the output unchanged on failure. The caller owns the returned allocation and must call `free` once. The result is not initialized; assign to elements before reading them.

## C.8 A consistent qsort comparator

```c
static int compare_ints(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}
```

Returning `a - b` could overflow for extreme signed values, invalidating the intended ordering. Use the comparator with `qsort` only when both pointers refer to valid `int` elements supplied by that call.

## C.9 A non-modifying failure test

Suppose a parser promises not to modify its output on failure. Before invoking it on an invalid string, set `unsigned result = 0x5a5aU;`. Verify the parser returns false *and* that the result remains `0x5a5aU`. Repeat with empty input, trailing junk, and an out-of-range number. A failed result alone does not establish the postcondition.

## C.10 A boundary test plan for a ring buffer

| Initial state | Operation | Required behavior |
| --- | --- | --- |
| Empty | Pop | Report false; do not change output |
| Empty | Push | Report true; count increases |
| One element | Pop | Return first item; become empty |
| Full | Push | Report false; preserve existing items |
| Partially consumed | Wraparound pushes | FIFO order is preserved |
| Returned to empty | Pop | Report false again |

Sequential tests establish logical behavior, not thread safety. The atomic SPSC queue in Chapter 16 needs additional concurrent reasoning and testing.

## C.11 How to check an exercise solution

For each solution, write down preconditions, postconditions on both success and failure, possible arithmetic overflow, and ownership effects. Then test exact boundaries. If your implementation uses a library function, read the library contract rather than assuming it matches the function's name.

Solutions are examples of reasoning, not necessarily the only acceptable design. An alternative is valid when it meets the same explicit contract.


## C.12 A counted slice and its ownership contract

A counted slice has a data pointer and a length. It does not automatically own the referenced array. The caller must keep storage alive for any operation reading the slice.

```c
#include <stddef.h>
#include <stdbool.h>

struct int_slice {
    const int *data;
    size_t length;
};

bool slice_get(struct int_slice s, size_t index, int *out)
{
    if (out == nullptr || s.data == nullptr || index >= s.length)
        return false;
    *out = s.data[index];
    return true;
}
```

On failure, the function leaves `*out` unchanged. It requires that a nonempty slice identify at least `length` accessible elements; a misleading length is a caller contract violation, not something a simple C function can discover from the pointer.

A subslice also needs to avoid unsigned overflow. Its valid-range test should be `offset <= original.length && count <= original.length - offset`, not `offset + count <= original.length`. With an empty original whose data pointer is null, return a null pointer for an empty result rather than attempting arithmetic on a null pointer.

The complete program in `examples/slices.c` exercises these rules, including `SIZE_MAX` and one-past positions.

## C.13 Failure-injected vector growth

The vector in `examples/vector_checked.c` owns its element allocation. An append operation uses a function pointer to a resizing function so a test can force reallocation to fail at a chosen point.

```c
typedef void *(*resize_fn)(void *, size_t);

static void *deny_resize(void *old, size_t bytes)
{
    (void)old;
    (void)bytes;
    return nullptr;
}
```

After filling the vector's first allocation, save the pointer, length, capacity and last element. Invoke append with `deny_resize`. A correct strong-failure implementation returns false without changing any saved property.

The second test repeats the append with a real `realloc` implementation; it then verifies the new value, the count, and safe destruction. Crucially, it does not use the old saved pointer after successful reallocation, because that pointer may be invalid even when its bit pattern looks unchanged.

This injection technique can be extended to `malloc` and to fake I/O sources. It is more reliable than hoping the operating system happens to run out of memory during a small test.

## C.14 Explain a failing test

A failed assertion is a starting point for diagnosis. Before modifying source, identify whether the program violated its own contract, the test made an invalid assumption, or the compiler did not implement the chosen language/library facility.

For example, a compiler warning that classifies a standardized C23 binary literal as a vendor extension does not automatically mean that the source is nonconforming. The response may be to use a more recent compiler or to suppress that one incorrect diagnostic while retaining other warnings. By contrast, using a macro removed in C23 is a source compatibility problem and should be corrected in the example itself.

A good verification record contains the failing command, exact diagnostic, cause, smallest corrective change, and the result of running tests on the corrected version. Avoid claiming an entire book has been verified when only a single executable has been compiled.

## C.15 A bounded frame encoder and decoder

A frame format should be described in bytes before it is represented by C types. The Chapter 14 example uses a magic octet, version octet, two little-endian length octets, zero to 64 payload octets, and an XOR checksum.

The encoder's preconditions include a writable destination of the declared capacity and a readable payload of the declared length. It rejects payloads longer than 64 and destinations smaller than `5 + length`. Because the maximum length is already fixed at 64, the total frame size can be proven representable on every target supported by the example. It constructs the complete frame in a local candidate buffer and then copies it to the caller's destination; this preserves the destination on validation failure.

The decoder checks fixed header size before reading fields, validates format identifiers, verifies that the declared payload matches the complete input length, and checks the checksum. Only after every check passes does it publish its local candidate result.

**Testing:** Start with a manually computed empty frame, then test a full 64-byte payload. Corrupt one payload byte without updating the checksum. Truncate a frame by one byte. Supply an extra byte. Confirm the output remains unchanged after rejection. The complete program is `examples/frame_codec.c`.

An XOR checksum offers limited accidental error detection, not sender authentication. A stronger checksum such as a CRC addresses some error patterns but is also not cryptographic authentication.

## C.16 A partial-write callback

The byte sink in Chapter 17 returns the number of bytes accepted in a call. A complete-write wrapper must never advance by more than the remaining count, and must stop when the sink reports no progress.

Let `offset` be bytes successfully accepted and `count` the original request. Maintain the invariant `offset <= count`. On each loop iteration, ask the sink for `count - offset` bytes starting at `data + offset`. Reject a returned value of zero or a value greater than requested. Otherwise add the positive count to `offset`.

Every successful iteration strictly increases `offset`, proving termination for a bounded input. This proof says nothing about how long a callback blocks; an RTOS deadline requires additional assumptions about the device operation.

The program `examples/callback_sink.c` uses a fake writer accepting at most two bytes per call. A five-byte message therefore requires three calls. A second mode always returns zero so the wrapper can demonstrate failure without looping indefinitely.

An asynchronous writer needs a richer design: a 'would block' result may be normal, and the wrapper may have to retain state across calls. Never use the synchronous implementation as an automatic substitute for an interrupt-safe or nonblocking API.

## C.17 Separate compilation as a testable contract

The module example consists of `examples/modules/checksum.h`, `checksum.c`, and `main.c`. The header declares a function taking a byte pointer, count, and output pointer. The implementation includes that header so an incompatible definition is diagnosed at compile time. The client also includes the same header and verifies that an invalid argument leaves the result unchanged.

The checksum is intentionally XOR, which is simple enough that the program's structure is more instructive than the calculation. The example should not be mistaken for a cryptographic hash.

A useful extra test is to compile `main.c` without linking `checksum.c`; the expected undefined-reference error illustrates the linker's job. Then compile `checksum.c` by itself with `-c`, compile `main.c` similarly, and link the object files.

A public header should include its own dependencies. It must not require clients to include `<stddef.h>` or `<stdbool.h>` before including it.

## C.18 What an example demonstrates—and what it does not

A passing byte-sink test demonstrates that the wrapper handles its tested partial-write and no-progress cases. It does not prove that a real UART driver handles DMA coherency or interrupt priority correctly. A passing frame-decoder test demonstrates behavior on its supplied packets, not exhaustive validation of every possible byte sequence. A passing module example demonstrates correct linkage under the chosen build command, not platform independence of every possible implementation.

For each exercise solution, separate observed test results from the assumptions and reasoning needed to use it safely in another environment. This distinction is essential in production embedded systems.


## C.19 Review checklist for the three complete examples

When reviewing `frame_codec.c`, verify that no length or checksum byte is read before its bounds check. Compare the decoded frame length with the precise number of supplied bytes, not merely with buffer capacity. Confirm that the encoder's temporary frame fits the fixed maximum and that a failed encode never writes into the caller's destination. The XOR checksum is not a security guarantee.

For `callback_sink.c`, prove that `offset` starts at zero, never exceeds the requested count, and increases on every successful loop iteration. A zero-byte callback result must terminate with failure rather than create a busy loop. The callback's promise not to return more than requested is checked; its promise to read only the provided source span remains part of the callback contract.

For the checksum module, compile the header in isolation and verify that it declares every type it needs. Link a test client with the implementation file, then intentionally omit the implementation and observe the expected linker failure. These exercises distinguish a language error, interface error, runtime test failure, and missing link dependency.

The important lesson is that tests, code review, and explicitly written invariants reinforce each other. A short, compile-tested demonstration can still require careful reasoning before its logic is reused in embedded production code.


## C.20 The exact boundary of a signed decimal parser

The most difficult valid input for a signed-decimal parser is often the negative endpoint, `LONG_MIN`. On a two's-complement implementation its magnitude is one greater than `LONG_MAX`, which means a positive signed accumulator cannot hold that magnitude.

The worked solution in `examples/parse_signed.c` instead accumulates an `unsigned long` magnitude. It selects the maximum permitted magnitude based on the sign: `LONG_MAX` for a nonnegative result and `(unsigned long)LONG_MAX + 1UL` for a negative result.

Before accepting each digit, it checks:

```c
if (magnitude > (limit - digit) / 10UL)
    return false;
```

This inequality is derived from the requirement that `10 * magnitude + digit <= limit`. The subtraction and division occur before the multiplication. The parser rejects an input once it would exceed the permitted mathematical range, so the accumulator never overflows.

At the final step, an exact negative endpoint is assigned as `LONG_MIN` rather than converted from an out-of-range positive signed value. For all smaller magnitudes, conversion to `long` is representable and a unary minus can safely be applied.

The grammar is also part of the contract: optional sign, at least one digit, no whitespace, no trailing characters. `+` and `-` by themselves are invalid. The output remains unchanged on failure.

**Boundary tests:** Use `snprintf` to create exact strings for `LONG_MIN` and `LONG_MAX` on the test machine. Add ordinary signed values, leading zeros, sign-only inputs, trailing garbage and much-too-long decimal strings. Confirm the output sentinel is not modified for any rejected string.

## C.21 A byte arena proof

Let the arena capacity be \(C\), its current used count be \(u\), and the requested count be \(n\). The invariant is \(0 \le u \le C\). A request for \(n>0\) bytes is accepted only when \(n \le C-u\).

The expression `C - u` is safe because the function first checks `u <= C`. It follows that \(u+n \le C\), so the updated offset is valid and the returned interval fits in the backing array.

```c
if (arena->used > ARENA_CAPACITY ||
    count > ARENA_CAPACITY - arena->used)
    return false;
```

The routine computes the returned pointer only after the checks. A failed request leaves both the `used` field and the out-parameter unchanged.

A reset changes `used` back to zero. It does not destroy the underlying byte array, but the allocator's logical contract makes all old borrowed regions invalid for subsequent use. Those bytes may be overwritten by new allocations. This distinction should be explicit in the interface documentation.

The example is a **byte** arena. It does not promise the alignment or object-access rules required for arbitrary typed allocations.

## C.22 Rotations without undefined shifts

For a 32-bit unsigned value, a left rotation by a count \(n\) in the range 1–31 consists of `(x << n) | (x >> (32 - n))`. The count zero must be handled separately, since shifting a 32-bit operand by 32 is invalid.

Normalize any supplied count with `n = count % 32u`. If zero, return the original value. Otherwise both shift counts are in the interval 1–31, satisfying the language's shift-count requirement.

The companion `examples/rotate_bits.c` tests the following independent constants:

| Input | Rotation | Expected value |
| --- | --- | --- |
| `0x12345678` | left 0 | `0x12345678` |
| `0x12345678` | left 4 | `0x23456781` |
| `0x12345678` | right 4 | `0x81234567` |
| `0x12345678` | left 32 | `0x12345678` |

It also checks that rotating left and then right by equal counts restores the original value for 64 count choices. That round-trip property is valuable but should not replace the independent constants, because two incorrect inverse functions could agree with each other.

## C.23 Equality and padding

Suppose two `struct event` values contain equal member values. Can `memcmp(&a, &b, sizeof a) == 0` be relied upon to express semantic equality? No. Structure padding bytes are not generally meaningful members, and their stored representations need not be identical. Floating-point members and pointers can add representation-specific concerns as well.

Define equality by comparing the meaningful fields under an explicit application policy. For floating fields, decide whether NaNs and signed zero need special treatment. For a string pointer, decide whether equality means the same pointer identity or identical string contents.

A wire format requires an additional design step: explicitly encode field widths, order and validity values. A raw structure copy is not a general serialization scheme.

## C.24 The ownership trace

Consider an owning vector `a`, a shallow copy `b = a`, and a borrowed pointer `p = a.data`. Initially all three pointer values refer to one allocation.

If `a` grows successfully through `realloc`, the old allocation's lifetime ends, even if the returned pointer value appears unchanged. The borrowed pointer `p` is invalid for accessing the old allocation. The pointer inside `b` is likewise invalid. Destroying `b` would attempt to release storage it no longer owns correctly.

If the vector instead fails to grow, the old allocation remains valid and the strong failure guarantee leaves `a` unchanged. But the copy `b` is still a second apparent owner, so destroying both will remain unsafe. Avoiding shallow ownership copies is a separate rule from handling failed allocation.

A correct design may use an opaque owning type, an explicit deep-copy function or a transfer operation that leaves the source in an empty state. C allows these conventions but does not enforce them automatically.

## C.25 A reviewer’s test matrix

| Operation | Successful boundary | Rejection boundary | Preserve on failure? |
| --- | --- | --- | --- |
| Signed parser | `LONG_MIN`, `LONG_MAX` | too many digits | Yes, output unchanged |
| Byte arena | exactly remaining bytes | one byte too many | Yes; state unchanged |
| 32-bit rotate | counts 0, 31, 32 | all counts permitted via modulo | Not applicable |
| Counted subslice | ending exactly at length | offset greater than length | Yes; view unchanged |
| Vector append | growth succeeds | injected resize failure | Yes, all prior elements |
| Frame decoder | complete valid frame | truncated or corrupted frame | Yes; record unchanged |

A passing test for every listed row provides useful evidence. It does not establish arbitrary pointer validity, interrupt safety, or correctness for all possible inputs. Those conclusions require the API contracts and independent reasoning developed in the chapters.


## C.26 An overlong line does not become two records

Consider a maximum of 16 ordinary bytes per line. A line containing 17 bytes is invalid. If a reader returns immediately after observing its 17th byte, remaining bytes from that same logical line may still be in the stream. The next call could incorrectly treat those bytes as the beginning of another record.

A bounded line reader should define a recovery policy. The example in `examples/line_reader.c` continues reading and discarding bytes until newline or clean EOF when it encounters an overlong or malformed line. That makes the next call begin at a genuine record boundary.

It builds a candidate line in a fixed local array and commits the text and reported length only on `LINE_OK`. Any other status leaves both caller outputs unchanged.

The test deliberately arranges the sequence:

```text
"short\\n"
"12345678901234567\\n"  (17 bytes: reject)
"recovered\\n"
"1234567890123456\\n"   (16 bytes: accept)
"final"                   (unterminated EOF: accept once)
```

The test must confirm that the failed record did not modify the previously returned line and that `recovered` is read next. The exact-capacity record should be accepted, with a terminating zero written in the extra slot. After `final`, the following call returns clean end of input.

There is a nonobvious tradeoff. Memory consumption is bounded by the candidate buffer, but a read call that drains an arbitrarily long invalid record has no finite input-independent bound on execution time. An event-loop or real-time design should use incremental input with a work budget rather than a single blocking drain operation.

A NUL byte is rejected here because the interface returns a C string. A counted binary record API could accept that byte under a different contract.

## C.27 Relaxed atomic increments and join

The `thread_counter.c` example creates two workers, each performing 20,000 atomic increments. The mathematical result is 40,000, which fits the minimum standard range of unsigned int.

Why is `memory_order_relaxed` enough? The counter is the only shared data whose value matters. Every increment is atomic, and the program does not use the counter as a signal that unrelated non-atomic payload bytes are ready.

Thread joins ensure that the program reaches its final count check after workers have completed. The context must remain alive until all started threads are joined. If creating the second worker fails, the first still needs to be joined before returning.

**What the test does not establish:** It does not show that relaxed memory ordering suffices to publish ordinary data, that an atomic operation is interrupt-safe on every MCU, or that the program can meet a real-time scheduling deadline. Those are different claims with different evidence requirements.

## C.28 A one-shot publication proof

A producer writes an ordinary payload and release-stores a ready flag. A consumer acquire-loads the flag and reads the payload only after observing true. If the acquire reads from the release operation, the producer's preceding writes happen-before the consumer's subsequent reads.

The proof assumes one publication and no subsequent writes to the payload while the consumer reads it. Resetting the ready flag and overwriting the payload without a separate handoff can create a data race. Atomicity of the flag is not a general lock on every object around it.

A repeating producer-consumer queue requires a protocol for both data publication and slot reclamation. This is why the SPSC ring example has release/acquire relationships for both head and tail.

## C.29 C23 feature probes as executable specifications

A compiler flag selects a dialect, but it is not an exhaustive compatibility test. A feature probe should compile a minimal translation unit that actually uses the feature.

For `constexpr`, a scalar constant object initialized by a permitted constant expression can be verified through `static_assert`. For `_BitInt`, use the exact requested width and check the value range intended by the algorithm. For `#embed`, include a known small binary file and verify that the array contains the expected bytes and length.

For `<stdckdint.h>`, the test must also include and link the header's functionality:

```c
#include <limits.h>
#include <stdckdint.h>

int main(void)
{
    int result;
    if (ckd_add(&result, 10, 20) || result != 30)
        return 1;
    return !ckd_add(&result, INT_MAX, 1);
}
```

On overflow, the test checks the boolean indication rather than making assumptions about the prior contents of the result. A wrapper API promising strong failure safety should use a temporary result.

A failed feature probe may indicate unsupported syntax, a missing library header, an unresolved library function, or a runtime mismatch. Report which step failed. Do not quietly downgrade the language mode while claiming C23 verification.

## C.30 What a complete review report looks like

For one example, write a review under six headings:

**Contract:** accepted input, output and failure behavior.

**Invariants:** statements preserved through every iteration, including bounds and ownership.

**Resource use:** maximum storage, allocation behavior, work per call, and whether it can block.

**Portability:** exact language assumptions, required headers, compiler version and target features.

**Verification:** independent expected values, error injection, executed compiler flags and CI run.

**Limitations:** untested interleavings, unsupported platforms, and properties requiring hardware or normative review.

This report can be shorter than the program. Its value is making the conditions of correct use explicit.


## C.31 An incremental UTF-8 decoder

The central challenge in decoding UTF-8 from a stream is that a transport boundary may divide one encoded scalar value. A decoder that reads only complete strings can treat missing continuation bytes as failure, but a stateful decoder needs to return *incomplete* without discarding valid prefix information.

The `utf8_stream.c` example maintains three fields: a partial unsigned scalar accumulator, the minimum valid scalar value for the chosen sequence length, and the number of continuation octets still needed.

Its transition table is:

| Current state | Incoming byte | New state or result |
| --- | --- | --- |
| Ready | 00–7F | Complete ASCII scalar |
| Ready | C2–DF | Store five payload bits; expect one continuation |
| Ready | E0–EF | Store four payload bits; expect two |
| Ready | F0–F4 | Store three payload bits; expect three |
| Ready | Other | Invalid |
| Waiting | 80–BF | Append six bits; decrement remaining |
| Waiting | Other | Invalid; reset |
| Final continuation | Result in permitted scalar range | Complete; publish value and reset |
| Final continuation | Overlong, surrogate or too large | Invalid; reset |

The implementation does not commit `*out` while waiting or on malformed input. This is the same strong failure-postcondition pattern used throughout the book: accumulate into local or private state and publish an answer only when the complete candidate has passed validation.

When the producer reports that no more input will ever arrive, a state with `remaining > 0` means truncated UTF-8 input. It is not necessarily an error when the source is merely waiting for its next packet or DMA buffer.

**Independent test vectors:** E2 82 AC must produce U+20AC; F0 9F 99 82 must produce U+1F642; E0 80 80 must be rejected as overlong; ED A0 80 must be rejected as surrogate; F4 90 80 80 must be rejected as greater than U+10FFFF.

A useful property test splits the *same valid octet sequence* into different input chunks and compares the completed scalar sequence. The result should depend on the bytes, not on how they arrived.

### What the example does not do

It does not normalize Unicode, identify grapheme clusters, apply case folding, or choose replacement-character behavior for malformed input. It deliberately rejects malformed sequences and resets state, which is a reasonable strict-parser policy but not a universal text-rendering policy.

It is also not a length-counted entire-text decoder: it emits one scalar at a time. An application must decide whether to append, count, compare, or otherwise process each completed scalar.

## C.32 A bounded TLV decoder

For the TLV decoder, let `size` be the count of input octets and `cursor` the count already consumed. The critical invariant is `cursor <= size`.

Before reading the tag and length, require at least two unconsumed bytes:

```c
if (size - cursor < 2)
    return false;
```

The subtraction is defined under the invariant and cannot underflow. After reading two header octets, advance cursor by two; that remains within the input because the check already proved two octets were available.

Then validate the announced payload length against both limits:

```c
if (length > TLV_MAX_VALUE || length > size - cursor)
    return false;
```

The first comparison is an application storage limit; the second is an input-completeness check. They are not interchangeable. A length can be perfectly valid in the protocol but unavailable in the truncated buffer, or fully present in a huge input but larger than the application permits.

Once both checks pass, `memcpy` receives a valid, bounded source span and a fixed-capacity destination field. The candidate's field count increases only after the complete field has been validated.

### Why commit once?

The decoder builds a local `struct tlv_message candidate = {};` and copies it into `*out` only after it reaches the exact end of the input. This prevents partial configuration updates. For example, a message containing one valid field followed by a truncated second field must not overwrite a previously valid configuration with the first field alone.

The sample tests a valid two-field record, incomplete header, incomplete payload, declared value length above the limit, and excessive field count. Each rejection must leave the previous output values intact.

### Resource reasoning

There are at most four fields and each contains at most eight value octets. That bounds the work of a successful decode and bounds the candidate's storage. Input bytes beyond the fifth field header cause rejection without copying an unbounded amount of data.

The TLV message may represent an empty field sequence. The sample contract accepts a null source pointer when and only when the supplied input length is zero. This avoids constructing any pointer through arithmetic on null.

### Semantics beyond TLV shape

A structurally valid TLV stream may still be an invalid command: it might contain repeated mandatory tags, unsupported tag codes, or semantically inconsistent fields. A real protocol should validate those requirements separately, before committing application state.

A TLV parser does not authenticate the sender. Nor does it guarantee freedom from replay or malicious changes. Those are independent protocol and security concerns.

## C.33 A comparison of decoding contracts

| Contract | Line reader | UTF-8 stream | TLV decoder |
| --- | --- | --- | --- |
| Input unit | Hosted stream byte | One octet | Counted octet buffer |
| Output unit | Terminated line | One Unicode scalar | Structured message |
| Incomplete input | Waits or EOF handling | `UTF8_MORE` | Rejected as incomplete |
| Capacity limit | 16 bytes per line | Constant decoder state | 4 fields, 8 octets each |
| Failure output | Unchanged | Unchanged scalar | Unchanged record |
| Recovery | Drain to delimiter | Reset on error | Reject entire record |
| Environment | Hosted streams | Any octet source | Counted buffer |
| Security guarantee | None | Encoding validity only | Bounds and shape only |

The table shows why a general-purpose parser interface is not one-size-fits-all. A blocking line reader owns the task of obtaining bytes. A one-byte decoder depends on the caller to provide the next byte. A whole-message TLV decoder requires that a framing component has already supplied the complete candidate record.

Each design can be correct when its preconditions and error semantics match the surrounding application.

## C.34 Turning an exercise into a reviewable result

For both examples, write a small verification report with these headings:

**Input grammar.** State exactly which byte sequences are legal.

**Invariants.** Identify the bounds and state relationships maintained after every step.

**Failure behavior.** State which outputs are unchanged, which parser state is reset, and which conditions require additional bytes.

**Limits.** Give the maximum allocated storage and maximum local parsing work, distinguishing those from transport waiting time.

**Evidence.** Provide independent test vectors, the compiler and language mode, and the resulting test run.

**Unverified properties.** Identify deployment-specific assumptions, unsupported Unicode operations, authentication, concurrency, and any untested code paths.

A reliable solution is one whose contracts can be reviewed without relying on lucky runtime behavior.

## C.35 Fixed pool allocation with generation-tagged handles

The fixed pool in examples/slot_pool.c manages four integer-valued records without a heap. Each slot has an occupancy flag, a retirement flag, a generation counter, and a value. A handle contains a slot index and the generation observed when the slot was allocated.

The central invariant is that a handle is valid only if its index is within the array, the slot is occupied, and the generations match. Checking just the index is insufficient: a freed slot may have been assigned to a different client.

### Acquire and release

An acquire operation scans at most four slots and chooses the first slot that is neither occupied nor retired. It writes the value, marks the slot occupied, and publishes the newly formed handle only after success. If no slot is available, the caller's output handle remains unchanged.

On release, the function first verifies the index, occupancy, and generation. It marks the slot unoccupied and increments its generation. An old handle now fails validation even if its original physical slot is reused for another value. A second release of the same handle is also rejected.

The implementation retires any slot whose generation has reached UINT_MAX. Ordinary unsigned arithmetic would wrap to zero, which could eventually recreate a previously issued handle identity. Retirement prevents that reuse at the cost of reducing the available pool capacity after extremely many cycles.

### A worked trace

| Event | Slot 1 generation | Occupancy | Old handle valid? |
| --- | --- | --- | --- |
| Initially | 0 | Free | No handle |
| Acquire value 20 | 0 | Occupied | Yes, handle (1,0) |
| Release | 1 | Free | No |
| Acquire value 99 | 1 | Occupied | No; new handle is (1,1) |
| Release again | 2 | Free | Neither old handle is valid |

The implementation tests a full pool, failed acquire, successful release, stale read, double release, reuse at a new generation, and retirement at the maximum generation.

### Limits of generation handles

A generation number is not authentication. A client can manufacture an index and number that happen to match. The teaching handle also lacks an owning-pool identifier; a handle from another pool may coincide with a valid pair. If clients are untrusted or handles cross subsystem boundaries, a stronger identity and permission scheme is necessary.

The pool is not thread-safe. Concurrent acquire operations can select the same available slot unless appropriate synchronization or task confinement prevents interleaving. The lack of dynamic allocation does not remove the need for ownership discipline.

## C.36 Worked byte-at-a-time frame parser

The parser in examples/frame_stream.c accepts frames with this layout:

    A5 | length | zero to eight payload octets | checksum

The checksum is XOR of length and every payload octet. It is intended only as a teaching error-detection mechanism, not authentication.

The state machine has four phases: wait for magic, read length, collect payload, and check checksum. Every call consumes one input octet and performs a bounded amount of work.

A complete message is published only when the checksum succeeds. While a frame is incomplete, the parser updates its private candidate and returns FEED_MORE without changing the caller's previously completed message. Malformed input resets the parser and returns FEED_REJECTED, again without publishing partial output.

### Independently calculated vector

A length-three payload containing hexadecimal octets 0A, 14, and 1E has checksum 03, because 03 XOR 0A XOR 14 XOR 1E equals 03. The full frame is therefore:

    A5 03 0A 14 1E 03

The first five feed calls should return FEED_MORE; the sixth returns FEED_COMPLETE and publishes three payload octets. An empty frame is A5 00 00 and should also complete.

### Bounds proof

While collecting payload, the declared length is in the interval one through eight. Before storing the next byte, the received count is smaller than the declared length. Therefore the destination index is less than eight and lies within the fixed array.

The parser accepts no length greater than eight. No loop runs over an untrusted length inside one feed operation. This supports a bounded per-byte work argument, though the actual MCU execution time still depends on the compiler, memory system, scheduling, and interrupts.

### Truncation and resynchronization

A prefix such as A5 02 07 is incomplete. The parser can retain its private state between input fragments, or the caller can reset it after a definite EOF or timeout.

A rejected checksum is consumed even if that byte coincides with the magic marker A5. The simple recovery policy can therefore miss the start of a possible following frame. The external protocol must explicitly define how resynchronization works; scanning blindly for the next magic octet is not always correct when that value may appear in payload data.

## C.37 The distinction between state and published results

The two examples illustrate different failure guarantees. A failed pool acquire preserves both pool availability and the caller's output handle. A stale release preserves all pool state. A frame feed returning FEED_MORE changes its internal parser state intentionally while preserving the public output. A rejected frame resets its internal state while still preserving the public output.

Consequently the phrase "no change on failure" is incomplete unless it specifies the object and state whose contents are unchanged.

For a large system, write down this distinction for every API: which state is private, which is externally visible, and exactly when a completed result is published.

## C.38 Concurrency and shutdown review

These examples are deterministic when one owner controls each mutable object. A real embedded application must also decide who feeds the parser, who owns the pool, and what happens when a producer outruns a consumer.

A common architecture places incoming UART bytes into a bounded ISR-safe queue and processes them in a normal RTOS task. The task can own the frame parser, while clients request pool operations through a controlled interface. This avoids mixing task and interrupt contexts indiscriminately.

The queue itself still needs a suitable overflow policy, its atomic operations must be supported on the target, and shutdown must ensure no execution context continues using released state.

### Extended exercises

**C.1** Add an owning-pool identity to handles and document how identity changes when a pool is reinitialized. **C.2** Add detailed parser statuses for invalid length, checksum failure, and incomplete input. **C.3** Test every valid payload length from zero to eight with independent expected frames. **C.4** Design the reset and recovery action after a simulated input-queue overflow. **C.5** Write a review explaining which properties are tested, which follow from invariants, and which require real hardware verification.


## C.40 A comparator that cannot overflow

Suppose an application stores events with priority, name, and arrival order. It wishes to sort by ascending priority, then alphabetically by name, then by increasing arrival number.

The tested `examples/sort_records.c` defines a comparator for `qsort`. Its integer comparisons follow this pattern:

```c
return (a > b) - (a < b);
```

For any two integer values, each relational expression yields either zero or one. Their difference therefore lies in the interval −1 through 1. The expression cannot overflow as an `int`.

Do not use `return a - b;` as a general signed-integer comparator. If `a` is `INT_MIN` and `b` is `INT_MAX`, the mathematical subtraction cannot fit in an `int`. Its undefined signed overflow makes that comparator invalid even though it may seem to work in ordinary cases.

The multi-key comparator returns as soon as it finds a differing key. For equal priorities it calls `strcmp` on the two stored names; only if those names compare equal does it compare the arrival numbers.

This establishes a lexicographic order. A test constructs an explicit expected arrival sequence, sorts the records, and compares each output position against that independent expectation. It also confirms that each adjacent pair is in ascending comparator order.

### Stability versus a tie-breaking key

ISO C's `qsort` does not promise that records comparing equal retain their original positions. Including the unique original arrival number as a final key distinguishes those records. Their relative positions then follow the comparator's specified order.

This does **not** make the sorting algorithm stable. It defines a total ordering that already contains the desired arrival-order behavior.

If arrival values are not unique, the comparator can still report equality for distinct records. Whether that is acceptable depends on the application's output contract.

### The comparator's pointer types

The `qsort` function passes two pointers to array elements as `const void *`. The callback converts them into `const struct record *` because the array being sorted contains records of that type.

A callback that interprets them as the wrong object type may violate the language's access rules or produce meaningless comparisons. C does not dynamically attach a type tag to a `void *`; the caller and callback must agree through the API contract.

The `strcmp` call also assumes both fixed-size name arrays contain terminating zero bytes. The teaching initializer satisfies that condition. A parser that populated names from untrusted packets would need to validate or supply the terminator before sorting.

### Extension

Add a fourth key, `timestamp`, and write the complete sort specification in prose before implementing it. Test equal priorities, equal names, equal timestamps, and different arrival values. A reviewer should be able to derive the expected array order from the stated contract independently of the code.

## C.41 A type-selected operation that evaluates once

The companion `examples/generic_select.c` uses `_Generic` to choose between ordinary typed functions:

```c
static int maximum_int(int a, int b)
{
    return a > b ? a : b;
}

static long maximum_long(long a, long b)
{
    return a > b ? a : b;
}

#define max_same_type(a, b) \
    _Generic((a), int: maximum_int, \
                  long: maximum_long)((a), (b))
```

The controlling expression `(a)` determines which function is chosen by type. It is not executed a second time as part of that selection. The arguments to the chosen function are evaluated as for an ordinary function call.

A crucial test is:

```c
int side_effect = 0;
int result = max_same_type(++side_effect, 7);
```

The expected `side_effect` after the call is one, and `result` is seven. This contrasts with the dangerous textual macro `#define MAX(a,b) ((a)>(b)?(a):(b))`, which can evaluate the chosen argument more than once.

The standard generic selection syntax was introduced before C23, but remains useful in modern C code. The standalone program is built under C23 alongside the other examples.

### The unstated conversion trap

The macro chooses its function using the type of `a` alone. If a caller passes an `int` as `a` and a `long` as `b`, the selection chooses the `int` function. The second argument is then converted to `int` through the function prototype.

Such conversion may discard information when the `long` value lies outside the range of `int`. The macro is therefore documented for arguments of matching supported types.

This is an important lesson in API design: compile-time type selection is not automatic enforcement of every relationship between two arguments.

### Additional type support

Extend the example with an unsigned function:

```c
static unsigned maximum_unsigned(unsigned a, unsigned b)
{
    return a > b ? a : b;
}
```

Add an association for `unsigned` and test values near `UINT_MAX`. Then try using a `double` first argument and observe the diagnostic for an unsupported association.

A production application can offer a named function for every important type. A small generic macro is worthwhile when it makes those existing functions easier to call without hiding the required value conversions.

## C.42 A complete three-file compilation exercise

The checksum module consists of:
- `examples/modules/checksum.h`, containing the public declaration;
- `examples/modules/checksum.c`, containing the definition;
- `examples/modules/main.c`, containing an executable test client.

The header must include `<stdbool.h>` for `bool` and `<stddef.h>` for `size_t`. It must not rely on the caller having included those declarations first.

A clean teaching build compiles both source files, then links their object files:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c examples/modules/checksum.c
cc -std=c23 -Wall -Wextra -Wpedantic -c examples/modules/main.c
cc checksum.o main.o -o modules_demo
```

The commands are illustrative for a `cc` compiler and should be run in a clean scratch directory because their default object names may overlap other files.

Remove `checksum.o` from the final link. The caller still has a valid declaration, but the linker cannot find the definition. This demonstrates an unresolved external symbol, a different problem from an unknown function declaration.

Next deliberately change the function's output parameter type in the implementation without updating the header. Because the implementation includes the header, the compiler can diagnose the conflict.

Finally, modify the checksum algorithm to exclude the final byte. Both source files may compile and link successfully, but the executable test's expected checksum should now fail. That is an algorithmic regression detected at execution rather than a compile/link interface error.

## C.43 Choosing evidence for the claim

A sorted-record example that passes deterministic tests provides evidence about its selected records and chosen ordering rules. It does not prove its comparator will never be given unterminated names by some future caller.

A generic macro that runs correctly for `int` and `long` does not establish safe behavior for every mixed-type call.

A successful two-file link does not establish that host and embedded targets use the same ABI or standard library.

When documenting any such example, separate the verified build and runtime observations from the assumptions under which the code remains correct. That distinction is particularly valuable when readers transfer a concise textbook example into a larger real-time or embedded application.
