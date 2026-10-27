# 18. Text, Encodings, and Robust Parsers

Text is often treated as the simplest form of input, but a text interface combines several independent conventions: how bytes are encoded, what counts as a character, where a line ends, and which text represents a valid value. C gives programmers byte-oriented primitives; a program must choose and document its interpretation of those bytes.

## 18.1 The byte is not the character

A C string is a sequence of `char` values terminated by a zero byte. The library function `strlen` counts units before the terminator. For UTF-8 text, that count is not necessarily the number of Unicode scalar values or user-perceived characters.

UTF-8 encodes a code point in one to four octets. Continuation octets do not stand alone. A decoder must reject overlong sequences, surrogate code points, values beyond U+10FFFF, and incomplete sequences. A robust decoder also must not read past the caller's buffer when the leading octet suggests more input than remains.

## 18.2 Parse lengths explicitly

An untrusted message need not have a terminating zero byte. A parser can accept a pointer and a length, so no operation depends on searching outside the supplied region.

```c
#include <stddef.h>
#include <stdbool.h>

bool ascii_digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}
```

This function classifies one byte without invoking locale-sensitive character classification. For applications that require whitespace recognition under a locale, `isspace` is useful, but its argument must be `EOF` or a value representable as `unsigned char`.

## 18.3 Decimal parsing as a checked transition

A decimal parser maintains an accumulated number. Each incoming digit requests a transition from `value` to `value * 10 + digit`. For an unsigned maximum `MAX`, that transition is valid only when

\[
value \leq \left\lfloor\frac{MAX-digit}{10}\right\rfloor.
\]

The inequality must be tested *before* multiplication. A checked parser can therefore reject overflow without ever evaluating the overflowing expression.

A complete implementation appears in `examples/parse_unsigned.c`. Its contract is intentionally narrow: only nonempty ASCII decimal digits are allowed, and failure leaves the output unchanged. This is different from `strtoul`, which has defined syntax for optional whitespace, signs, and bases and uses `errno` for range errors.

## 18.4 A bounded UTF-8 decoder

The companion program `examples/utf8_decode.c` decodes a single Unicode scalar value from a counted input buffer. It reports how many octets were consumed on success and does not change either output on failure.

For example, the sequence `E2 82 AC` represents U+20AC (the euro sign). But `C0 80` must be rejected: it is an overlong representation of U+0000. The byte sequences `ED A0 80` and `F4 90 80` must also be rejected because they denote disallowed scalar values.

The example's purpose is decoding, not Unicode normalization, segmentation, case folding, or character rendering. Those operations require additional specifications and, in real-world applications, frequently a carefully selected text library.

## 18.5 Failure and incomplete input are distinct

A streaming decoder may encounter a prefix of a potentially valid sequence, such as the first two octets of a three-octet UTF-8 character. A one-shot parser can report failure and ask its caller to provide more bytes. A streaming parser often benefits from three results: valid, invalid, and incomplete.

That distinction prevents a network receiver from prematurely rejecting input merely because it arrived in pieces. It also prevents an actual malformed sequence from being treated as indefinitely incomplete.

## 18.6 The language's execution character set

C programs have implementation-defined character representations for source and execution environments, within the language's requirements. The ordinary literal `'A'` does not universally imply the integer 65. An ASCII-oriented parser must state its ASCII requirement explicitly.

The examples in this chapter use octet values for UTF-8 and the digit range for decimal input. Such assumptions are appropriate when decoding formats defined in ASCII or UTF-8, but they are not claims about every C execution character set.

## 18.7 A secure parsing checklist

Before parsing an untrusted input, identify its maximum length, byte encoding, permitted grammar, numeric ranges, truncation policy, and error reporting. Validate a complete candidate before writing durable output state. Avoid relying on null terminators unless the interface guarantees them.

Tests should include zero-length input, all valid boundary values, incomplete prefixes, extra trailing bytes, and malformed encodings. Pair handwritten test vectors with fuzzing, and preserve discovered failures as regression tests.

## 18.8 Exercises

**18.1** Implement a parser for unsigned hexadecimal digits with an explicit buffer length and checked overflow.

**18.2** Extend the UTF-8 decoder to distinguish incomplete input from malformed input.

**18.3** Explain why byte length, Unicode scalar count, and displayed character count can differ.

**18.4** Add fuzz tests checking that malformed input never changes the decoder's outputs.

**18.5** Design a streaming line protocol that accepts CRLF and LF line endings while bounding the line length.

**18.6** Compare a locale-sensitive classification function with a format-specific ASCII byte test.


## 18.9 UTF-8 validity is more than continuation bytes

A four-byte UTF-8 sequence is not valid merely because its final three bytes begin with `10` in binary. The decoded value must also lie between U+10000 and U+10FFFF. A three-byte sequence must not encode a value below U+0800 or any surrogate in U+D800–U+DFFF. A two-byte sequence must not encode an ASCII value.

These rules reject overlong encodings and invalid Unicode scalar values. A robust decoder should validate both the structure of each sequence and the numerical range of its result before publishing it.

## 18.10 Character count versus display width

UTF-8 code points are not synonymous with visual characters. A base letter followed by a combining accent can use two scalar values but appear as one grapheme cluster. Some visible characters use multiple code points joined by special sequences. Consequently, a user-interface operation such as cursor movement or truncation by displayed character count needs Unicode segmentation rules beyond a basic decoder.

A low-level C parser can be correct while knowing nothing about grapheme clusters. Its contract should say that it counts bytes or scalar values, not vaguely claim to count characters.

## 18.11 Incremental decoding as a state machine

A streaming decoder can retain a partial code point, a count of expected continuation bytes, and enough information to reject an invalid range. Each input byte updates that state. The result can be one of three conditions: a scalar value completed, more bytes needed, or malformed input.

On an I/O boundary, incomplete is not necessarily invalid. If the stream ends while the decoder still expects bytes, the incomplete sequence becomes a final input error according to the protocol's policy. This mirrors the word-counting state machine from Chapter 1, but with stricter validity requirements.

## 18.12 Parsing machine-oriented text

Protocol text is not necessarily locale-dependent natural language. A command such as `SET 1234` may be explicitly ASCII with no whitespace variation; a JSON document is UTF-8 with its own grammar and escape sequences. Use a parser matching the format rather than applying broad locale-sensitive rules indiscriminately.

A counted span makes boundaries explicit. For an ASCII command field, scanning exactly its declared number of bytes avoids relying on an unrelated terminator. Reject embedded NUL bytes if the grammar forbids them, even though an ordinary C string function might stop at the first one and ignore trailing bytes.

## 18.13 Validate before normalizing

Converting malformed byte sequences to lowercase or replacing bytes before validating the input can introduce ambiguity. For format-specific ASCII tokens, validate each byte first, then normalize the allowed subset. For general Unicode text, decoding and normalization need a documented Unicode library and version.

**Further problems:** Extend the decoder to report incomplete separately from invalid. Produce golden test vectors for one-, two-, three-, and four-byte values. Design a text protocol that rejects embedded NUL and trailing garbage rather than silently ignoring it.


## 18.14 Separate three kinds of text boundaries

A byte stream has transport boundaries, an encoding has code-unit and code-point boundaries, and a user interface has grapheme or displayed-character boundaries. These are not interchangeable.

A UTF-8 character may be split across two `fread` calls or UART DMA transfers. The decoder must retain the incomplete prefix or the adapter must buffer sufficient bytes. Even after a valid scalar value has been decoded, two adjacent values may combine into a single user-perceived character. A simple MCU command parser often needs only the first two concepts, while a text editor needs all three.

A counted input buffer should always be processed according to its supplied length. A zero byte inside the buffer is valid as a byte value; whether it is allowed by the *protocol* is a separate grammar decision. Using `strlen` on arbitrary counted data can stop early at an embedded zero or read past the allocation if there is no terminator.

## 18.15 Build the UTF-8 validity proof

For one-byte sequences, the leading byte lies in 00–7F. Valid two-byte sequences begin with C2–DF; C0 and C1 are excluded because they would produce overlong encodings. Valid three-byte sequences begin with E0–EF but need additional restrictions on the second byte or, equivalently, a final decoded-scalar range check. Four-byte sequences begin with F0–F4 and likewise require a result no greater than U+10FFFF.

Every continuation byte must have its top two bits `10`. After combining all payload bits, reject decoded values below the shortest possible value for that sequence length. Reject surrogate code points U+D800–U+DFFF. These tests, in that order or an equivalent validated order, establish that the input represents a Unicode scalar value.

The standalone `utf8_decode.c` handles one code point and reports its consumed length on success. A complete streaming implementation needs an additional status to distinguish a truncated but potentially valid prefix from a definitely malformed sequence.

## 18.16 Avoid accidental locale dependencies

C's `<ctype.h>` functions operate on character values according to their documented rules and may be locale-sensitive. For a protocol defined in ASCII, the explicit range check `c >= '0' && c <= '9'` may make the grammar clearer, subject to the execution character representation. For raw UTF-8 bytes, interpreting every byte through `tolower` is incorrect Unicode processing.

A production system should choose its textual contract at its boundary: ASCII protocol tokens, UTF-8 source text, or locale-sensitive human input. Once chosen, the parser should not silently switch conventions halfway through processing.

## 18.17 Bounded streaming text

A command reader with a maximum length of 64 bytes needs both an accumulation buffer and a policy for overflow. If a command exceeds the limit, immediately terminating the connection may be appropriate for a strict protocol. Alternatively, it can discard subsequent bytes until a line delimiter and then resume with the next record.

Do not accept the first 64 bytes as a complete valid command: truncation can change semantics. A long dangerous command might share a harmless-looking prefix with a valid shorter command. Rejecting the entire overlong record preserves the intended grammar.

A streaming decoder also needs an EOF policy: an incomplete multibyte sequence at the end of a stream is not the same thing as clean EOF between complete records.

### Exercises

**18.7** List valid and invalid UTF-8 leading-byte ranges. **18.8** Build a state machine that distinguishes 'need more bytes' from 'invalid encoding'. **18.9** Explain why NUL may be a valid byte while still being forbidden inside a chosen text protocol. **18.10** Specify the overflow and resynchronization behavior of a bounded line reader.


## 18.18 A streaming decoder that remembers incomplete input

The one-shot decoder earlier in this chapter receives a buffer containing all bytes for one candidate scalar value. Actual input often arrives in arbitrary fragments. A UART interrupt might deliver one byte, while a socket read might deliver several complete characters followed by part of another. The parser must not equate a transport read boundary with a character boundary.

A streaming decoder can retain just three pieces of state:

- **Accumulator:** the bits of the scalar value built so far.
- **Minimum:** the smallest legal scalar value for the selected sequence length.
- **Remaining:** the number of continuation octets still required.

For ordinary ASCII (00–7F), decoding completes with one byte and no persistent state. For a multibyte leading octet, the decoder records an initial bit field and waits for its continuation octets. Each continuation contributes six more bits.

The implementation in `examples/utf8_stream.c` expresses this using a small state structure:

```c
struct utf8_decoder {
    uint32_t value;
    uint32_t minimum;
    unsigned remaining;
};
```

The decoder does not allocate memory and its state size does not grow with the input stream. That is important for constrained embedded devices and untrusted network input.

## 18.19 Three results are more useful than two

A one-byte-at-a-time decoder needs to distinguish:

| Result | Meaning | Output scalar |
| --- | --- | --- |
| `UTF8_MORE` | The prefix may still become a valid scalar, but more octets are required | Unchanged |
| `UTF8_COMPLETE` | One valid Unicode scalar has been decoded | Updated |
| `UTF8_INVALID` | The input violates the encoding rules, or API arguments are invalid | Unchanged |

A boolean result would conflate a valid but incomplete prefix with malformed input. That distinction matters in streaming I/O: receiving the leading octet E2 by itself is not malformed; receiving E2 followed by an invalid continuation is.

The implementation publishes `*out` only when a complete scalar has passed every validation. On any failure or incomplete prefix, the caller's earlier output remains unchanged.

That strong failure rule allows a caller to retain its last successfully decoded scalar without inventing a separate validity flag for the output storage.

## 18.20 Validate the leading octet first

UTF-8's leading-octet ranges determine the expected length:

```text
00..7F     ASCII scalar, complete in one octet
C2..DF     start of a two-octet sequence
E0..EF     start of a three-octet sequence
F0..F4     start of a four-octet sequence
other      invalid as a leading octet
```

C0 and C1 are not valid starting octets because they could produce overlong representations of ASCII values. The bytes 80–BF are continuation octets, not standalone leading octets. F5–FF are outside the permitted leading range for Unicode scalar values.

After choosing a sequence length, the decoder records a minimum allowed scalar value: 0x80 for two octets, 0x800 for three, and 0x10000 for four. These lower limits later reject overlong encodings without requiring dozens of special leading/second-octet cases.

## 18.21 Every continuation byte has a shape

A continuation octet begins with binary bits `10`. The test

```c
if ((byte & 0xc0) != 0x80)
    return UTF8_INVALID;
```

rejects any byte that is not a continuation when one is required. A correct decoder also resets its partial state on an invalid continuation according to a clearly documented recovery policy.

After validation, the low six bits are incorporated:

```c
value = (value << 6) | (byte & 0x3f);
```

This operation is performed on an unsigned accumulator. At most four UTF-8 octets are accepted for one scalar; the accumulated value is consequently bounded well below the maximum of `uint32_t`. The shift count is the fixed value six, valid for the type.

The decoder decrements `remaining`. If more continuation bytes are required, it returns `UTF8_MORE`. When the remaining count reaches zero, it performs final scalar-range validation.

## 18.22 A complete sequence is not necessarily a valid scalar

Even correctly shaped continuation octets can assemble an invalid scalar. A final validity check rejects:

1. Values below the minimum for their sequence length (overlong encodings).
2. Surrogate code points U+D800 through U+DFFF.
3. Values greater than U+10FFFF.

For example, E0 80 80 has three valid-looking octets but decodes to zero, which must be represented using the one-octet form 00. ED A0 80 decodes to a surrogate. F4 90 80 80 exceeds the maximum Unicode scalar.

The accompanying tests include all three cases. They also verify that a failure does not overwrite the previously decoded scalar.

Checking only the structural shape of octets is therefore insufficient.

## 18.23 End of stream is a separate event

A decoder may be sitting in a valid partial state when its input source reaches clean EOF. The byte prefix was not necessarily malformed when first encountered, but the now-finished stream does not contain a complete scalar.

A small finishing operation can ask whether `remaining == 0`. If it is nonzero, the complete input was truncated. A higher-level parser should report that condition according to its data format: an incomplete UTF-8 text file, a malformed protocol field, or an input requiring retransmission.

Do not silently discard the final incomplete prefix and report clean success, because truncation can change what a document or command means.

An incremental network protocol may keep the partial state between reads and call the finish check only when it has received an actual end-of-message indication. A temporary lack of available bytes is not automatically EOF.

## 18.24 Recovery is not universal

The example resets decoder state after a malformed sequence and allows the next supplied byte to begin a new scalar. That is a simple teaching policy. A replacement-character decoder may instead emit U+FFFD and consume a precisely defined maximal malformed subsequence. A strict protocol decoder may reject the entire surrounding record and refuse any recovery.

These are different interfaces. A caller must not assume that a particular number of bytes have been consumed on failure unless the decoder explicitly reports that fact.

Unicode normalization is also outside this example. Decoding two valid scalar values does not imply they form two independently displayed characters. Applications requiring canonical normalization or grapheme segmentation need additional Unicode-aware processing.

## 18.25 A complete test matrix

A good streaming test must challenge transitions, not only full strings:

| Input | Expected behavior |
| --- | --- |
| ASCII `A` | Complete after one byte |
| E2 82 AC | Two `MORE` responses, then U+20AC |
| F0 9F 99 82 | Three `MORE` responses, then U+1F642 |
| E2 at temporary input boundary | `MORE`, not malformed |
| E2 at final EOF | Incomplete final input |
| E0 80 80 | Reject overlong sequence |
| ED A0 80 | Reject surrogate |
| F4 90 80 80 | Reject value above U+10FFFF |
| E2 followed by `X` | Reject invalid continuation, reset state |

An important additional property is that a decoder's result should not depend on the way the same valid byte stream is divided into transport chunks. One byte at a time and multiple chunks should produce identical Unicode scalar values.

The provided example has no heap allocation and a constant-size state object. This makes its memory requirements predictable, but does not by itself prove bounded application latency if the surrounding code waits on I/O.

## 18.26 From scalar decoding to a full text pipeline

A production text-processing system may have the following pipeline:

```text
byte source -> framing -> UTF-8 decoding -> grammar parsing -> application
```

Each stage has different errors. The source can report read failure. Framing can reject an overlong record. UTF-8 decoding can reject malformed sequences. Grammar parsing can reject a command using an unsupported keyword. The application can reject a valid command that violates authorization or resource constraints.

Do not merge all these into a generic 'bad character' result when the caller needs to react differently. Conversely, a small application may deliberately map several low-level errors to one external response to avoid exposing unnecessary implementation detail.

Keeping the layers separate lets the same UTF-8 decoder operate on file input, a network stream, and data buffered by an RTOS task without depending on those transport mechanisms.

### Further exercises

**18.11** Adapt the decoder to accept counted chunks while preserving partial state between calls. **18.12** Design a result type that distinguishes invalid input from final truncated input. **18.13** Specify a policy for recovering from malformed UTF-8 using U+FFFD. **18.14** Verify that valid decoding results are independent of transport chunk boundaries. **18.15** Explain why even a valid UTF-8 scalar decoder cannot compute user-perceived character count without additional Unicode rules.
