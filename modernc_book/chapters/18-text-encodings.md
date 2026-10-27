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
