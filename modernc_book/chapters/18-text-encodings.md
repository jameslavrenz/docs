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
