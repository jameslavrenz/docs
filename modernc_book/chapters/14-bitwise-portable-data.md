# 14. Bits, Bytes, and Portable Data

C gives programmers direct control over integer representations and bytes. That power is especially useful for embedded devices, persistent files, and network protocols. But an integer's mathematical value is distinct from its in-memory representation, and a portable program must not confuse the two.

## 14.1 Storage units

An object occupies one or more bytes. In C, a byte is the size of a `char` and contains `CHAR_BIT` bits; while eight-bit bytes are overwhelmingly common, the language does not require `CHAR_BIT == 8`. A format specified in *octets* requires eight-bit units. A portable implementation must check that its target can represent and address the format as intended.

```c
#include <limits.h>
#include <stdio.h>

int main(void)
{
    printf("bits per byte: %d\n", CHAR_BIT);
}
```

Exact-width integer types such as `uint32_t` exist when the implementation offers a type of exactly the specified width without padding bits. Fixed-width names clarify the protocol's mathematical range, but they do not choose byte order.

## 14.2 Masks, shifts, and flags

Use unsigned values for bit-level work. A left shift with a count greater than or equal to the promoted operand's width has undefined behavior, as does a negative shift count. The shift operators have their own integer-promotion rules; explicitly selecting a wide enough unsigned operand avoids accidental signed arithmetic.

```c
#include <stdint.h>

static uint32_t extract_byte(uint32_t value, unsigned position)
{
    return (value >> (position * 8u)) & UINT32_C(0xff);
}
```

This helper requires the precondition `position < 4`. Without it, the shift count can be invalid. A production interface can check the position and return success through a separate status indicator.

A register read-modify-write operation may not have the semantics of an ordinary RAM variable. For instance, writing a one may clear a device status bit. Apply shifts and masks according to the hardware reference manual, not just the C expression's arithmetic meaning.

## 14.3 Endianness is part of the format

A little-endian 32-bit unsigned field stores its least significant octet first. A big-endian field stores the most significant octet first. For portable decoding, use shifts and masks on bytes rather than reinterpreting an input buffer as a pointer to `uint32_t`.

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool decode_u32_be(const unsigned char *src, size_t n, uint32_t *out)
{
    if (src == nullptr || out == nullptr || n < 4)
        return false;
    uint32_t v = ((uint32_t)src[0] << 24)
               | ((uint32_t)src[1] << 16)
               | ((uint32_t)src[2] << 8)
               | (uint32_t)src[3];
    *out = v;
    return true;
}
```

This assumes eight-bit input bytes as specified by the format. If the environment permits wider bytes, a protocol layer should explicitly define how octets are obtained. The output is assigned only after the bounds check.

## 14.4 Representation and alignment

An address in a byte stream may not meet the alignment required by an integer type. Converting that address and dereferencing a misaligned integer pointer may be undefined. Even on processors that allow unaligned loads, compiler assumptions or peripheral restrictions may differ. Byte-wise decoding avoids this issue.

Structure padding and bit-field layout may differ across implementations. Avoid transmitting raw structures as protocol records. Instead, define field lengths, byte order, permitted ranges, and checksums. Encode fields explicitly and test them against documented vectors.

## 14.5 The meaning of volatile

A memory-mapped device register may require volatile-qualified access so that the compiler performs observable reads and writes. But `volatile` does not specify the hardware bus transaction width, cache behavior, atomicity, memory ordering across cores, or synchronization with interrupts. Those require target-specific facilities and device documentation.

When two threads share ordinary application state, prefer atomics or appropriate synchronization rather than a volatile flag. The language's concurrency rules and a hardware peripheral's rules are related but not interchangeable.

## 14.6 A complete byte codec

See `examples/byte_codec.c`, which encodes and decodes a 32-bit value in big-endian octet order and verifies the result. It checks `CHAR_BIT == 8` at translation time and uses unsigned shifts. Its public-style functions take explicit output capacity and return `false` when storage is insufficient.

The tests exercise zero, a mixed-byte value, and the maximum 32-bit value. An actual protocol should also test malformed lengths, endianness against independent known vectors, and behavior when the caller supplies invalid storage.

## 14.7 Exercises

**14.1** Write `decode_u16_le` with an explicit length and output pointer, preserving the output on failure.

**14.2** Explain why `*(const uint32_t *)buffer` is not a portable replacement for a byte decoder.

**14.3** Implement a 32-bit rotate-left operation without shifting by the type's width. Test shift counts zero, one, 31, and 32 after defining whether counts are reduced modulo 32.

**14.4** Write a packet encoder with a documented maximum length, a version field, and a checksum. Prove that it never writes past its capacity.

**14.5** Compare a host byte codec with the actual peripheral register interface of a chosen microcontroller. Identify which assumptions belong to C and which belong to the vendor's device specification.


## 14.8 Fixed-width data is not fixed layout

Declaring `uint32_t word;` specifies a 32-bit unsigned integer type if the implementation provides it. It does not say which byte of its object representation occupies the lowest address. Nor does it authorize unaligned access to an arbitrary buffer address after casting it to `uint32_t *`.

A format that defines 32-bit fields in network byte order requires explicit big-endian encoding and decoding, regardless of whether the host processor happens to use little-endian loads. Keeping representation conversion in small functions makes protocol code easy to test on multiple architectures.

## 14.9 Shifts need a domain proof

The expression `1 << position` has a signed left operand. For sufficiently large `position`, the result may not fit, or the shift count may equal or exceed the promoted width. Using an unsigned fixed-width operand addresses the signedness issue but not invalid counts.

```c
#include <stdbool.h>
#include <stdint.h>

bool make_u32_mask(unsigned position, uint32_t *out)
{
    if (out == nullptr || position >= 32)
        return false;
    *out = UINT32_C(1) << position;
    return true;
}
```

This example rejects positions outside the 32-bit domain before performing the shift. It does not assume a particular memory representation of `uint32_t`.

## 14.10 Check the complete frame size

Suppose an output frame contains a four-byte header, a payload, and a one-byte checksum. If the caller gives a buffer of capacity `capacity`, it is tempting to test `capacity >= payload_length + 5`. But that sum may overflow `size_t`.

A safer shape is:

```c
if (capacity < 5 || payload_length > capacity - 5)
    return false;
```

Only after this check is it safe to index all fields in a correctly allocated buffer of the declared capacity. This is an arithmetic *and* storage contract: the caller must actually provide a valid region that large.

## 14.11 MMIO is not just a pointer cast

In embedded programming, a peripheral register may be accessed through a volatile-qualified lvalue or vendor-provided accessor. The C language does not itself specify the address map, the side effects of reading, or whether writing a one clears a flag. The device manual and compiler/ABI documentation remain authoritative for such accesses.

A compound assignment like `REG |= MASK` may compile into a read followed by a write. It may be unsuitable for a register whose read has side effects, whose reserved bits must retain particular values, or whose updates must be atomic with respect to an interrupt. Some peripherals offer dedicated set/reset registers; use the documented mechanism.

## 14.12 Field tests beat raw dumps

For a byte codec, test zero, all bits set, alternating-bit patterns, and a distinct-byte value such as `0x12345678`. The distinct-byte case is especially useful for finding reversed byte order. Then test output buffers that are too small, null pointers when prohibited, and any nonstandard target assumptions.

**Further problems:** Write an encoder for a 24-bit unsigned field stored in three octets. Show that each shift count is valid. Explain why an ordinary struct copy and a wire serialization are different operations. Analyze a hypothetical write-one-to-clear status register.

## 14.13 A wire protocol is a specification, not a structure

Suppose a device transmits a frame with this byte-level layout:

| Offset | Width in octets | Meaning |
| --- | --- | --- |
| 0 | 1 | Magic value `0xA5` |
| 1 | 1 | Version `0x01` |
| 2 | 2 | Unsigned payload length, little-endian |
| 4 | 0–64 | Payload |
| 4 + length | 1 | XOR of all preceding octets |

This is an intentionally simple teaching format. The XOR field detects some accidental errors; it does not authenticate a sender and is not adequate against deliberate modification.

The format does not mention `struct`, padding, alignment, or host byte order. Those are properties of the implementation, not the external message. A portable encoder explicitly writes each field as bytes. A portable decoder reads each field only after establishing that the corresponding bytes exist.

## 14.14 Encode without partial publication

An encoder can reject an invalid request before writing any output: reject a null destination, insufficient capacity, a payload length beyond 64, and a nonzero payload length with no valid payload storage. Then compute exactly how many bytes are needed, using arithmetic already bounded by the format's maximum.

If failure must leave the destination unchanged, construct the complete frame in a local fixed-size temporary array and copy it only after validation. That costs one extra bounded copy but gives callers a simple postcondition. For larger frames, an alternative API may write incrementally and report partial output; either policy is acceptable if explicit.

The companion `examples/frame_codec.c` demonstrates a strong-failure interface for both encoding and decoding. It uses `memcpy` only after checking capacities. Its local buffer size is determined by the maximum defined frame size rather than by untrusted input.

## 14.15 Decode in a safe order

For decoding, perform these checks in sequence:

1. Confirm that the input pointer and output pointer satisfy the API and that at least five octets exist.
2. Check magic and version fields.
3. Decode the two-byte length from individual octets.
4. Ensure length is at most 64 and the input contains exactly the declared frame length.
5. Verify the checksum.
6. Copy payload bytes into a local candidate and commit the result.

If the function subtracts a fixed overhead from the input length, it must first establish that the input length is at least that overhead. This prevents unsigned underflow. Because the maximum payload is small, the final length calculation can be proven to fit in `size_t` on supported targets.

## 14.16 Endianness and alignment are independent

A machine can allow unaligned access and still have a byte order different from the protocol. Conversely, a target may use the protocol's byte order but require alignment for native multi-byte loads. Casting the input pointer to a multi-byte integer pointer therefore makes more than one unsafe assumption.

The explicit decoder `(uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8)` communicates the wire format and avoids reading a misaligned native integer. Since `uint16_t` might undergo integer promotions, the operation remains carefully bounded by the known eight-bit octet values.

## 14.17 Testing message boundaries

Test a zero-length payload, a payload at maximum length, one byte less than a complete frame, an extra trailing byte, a corrupted checksum, and an unsupported version. Confirm that failures leave the output object unchanged. Also test against a manually constructed frame rather than relying exclusively on an encoder/decoder round trip.

### Exercises

**14.6** Change the checksum from XOR to a documented CRC and provide independent golden test vectors. **14.7** Add an error enumeration for malformed, incomplete and oversized frames. **14.8** Design a streaming decoder using bounded state and explain how it handles resynchronization after corrupted input.
