# 5. Structures and Data Representation

Structures associate fields under one type. Enumerations name integral alternatives. Unions allow members to share storage, but they require disciplined tracking of which interpretation is meaningful.

## 5.1 A value type

```c
struct sample {
    unsigned channel;
    double measurement;
    bool valid;
};

struct sample reading = {
    .channel = 2,
    .measurement = 3.75,
    .valid = true
};
```

Designated initializers name members instead of depending on their order. Structure assignment copies member values. Padding bytes may exist between fields; they are not stable serialized data. Do not write a structure directly to a wire protocol or disk format and assume portability across targets.

## 5.2 Tagged unions

```c
enum value_kind { VALUE_INTEGER, VALUE_REAL };

struct value {
    enum value_kind kind;
    union {
        long integer;
        double real;
    } data;
};
```

The tag establishes which member the surrounding program treats as active. An API should expose constructors and accessors rather than relying on callers to maintain an undocumented convention.

## 5.3 Linked records

```c
struct node {
    int key;
    struct node *next;
};
```

A linked list illustrates the difference between a logical collection and its physical representation: nodes may be discontiguous, each allocation may fail, and traversal follows explicit links. In embedded systems, a bounded array can often be simpler and more predictable than a heap-backed list.

## 5.4 Bit fields and explicit masks

Bit fields can make local state concise, but details of layout and allocation are implementation-defined. For device registers and portable serialized formats, fixed-width unsigned integers with shifts and masks often make data representation clearer. Hardware access adds concerns about `volatile`, synchronization, barriers, and the device's register semantics; `volatile` alone does not make an operation atomic.

**Exercises.** 5.1 Represent a timestamp with a structure and validate its fields. 5.2 Implement a fixed-capacity pool of list nodes without `malloc`. 5.3 Encode two unsigned 4-bit fields into a byte and decode them.


## 5.5 Field order, padding, and ABI

A structure establishes a logical association among members, but not a portable wire representation. A compiler may insert padding to meet alignment requirements. The order of named non-bit-field members is preserved in address order, yet `sizeof(struct record)` may exceed the sum of member sizes.

```c
struct record {
    unsigned char kind;
    unsigned count;
};
```

There may be padding between `kind` and `count`, or after the final member. Different target architectures or compiler options may create different representations. Avoid `fwrite(&record, sizeof record, 1, stream)` as a long-term file format unless the representation is intentionally fixed for that exact environment.

To serialize an unsigned multi-byte value, select an exact format and explicitly encode bytes. That code is easier to test and more portable than depending on host endianness or struct layout.

## 5.6 A bounded ring buffer

A ring buffer holds a fixed number of elements and wraps its index back to the beginning. We use a separate count to distinguish an empty buffer from a full one.

```c
#include <stdbool.h>
#include <stddef.h>

enum { QUEUE_CAPACITY = 8 };

struct queue {
    unsigned data[QUEUE_CAPACITY];
    size_t head;
    size_t count;
};

bool queue_push(struct queue *q, unsigned value)
{
    if (q == nullptr || q->count == QUEUE_CAPACITY)
        return false;
    size_t tail = (q->head + q->count) % QUEUE_CAPACITY;
    q->data[tail] = value;
    ++q->count;
    return true;
}

bool queue_pop(struct queue *q, unsigned *out)
{
    if (q == nullptr || out == nullptr || q->count == 0)
        return false;
    *out = q->data[q->head];
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    --q->count;
    return true;
}
```

Initialize the entire queue with `struct queue q = {};`. This implementation is appropriate for sequential calls; it is **not** a concurrent lock-free queue. Sharing it between an interrupt and a thread introduces separate ordering, atomicity, and scheduling requirements.

One invariant states that `count` stays between zero and capacity. Another states that `head` is a valid index. Both remain true after successful and failed operations, assuming a properly initialized queue.

## 5.7 Opaque structures as a design choice

Sometimes callers need a type's behavior, not its layout. A header may declare `struct handle;` without defining its members. A source file privately defines the structure and exports functions taking pointers to it. This allows implementations to change without recompiling all callers, though allocation and ownership must be carefully arranged.

Opaque types are especially useful for devices, parsers, and protocol sessions. A public, by-value structure is often preferable for tiny value types that need no hidden invariants.

## 5.8 Exercises

**5.4** Trace the queue above across eight pushes, one failed push, three pops, and three more pushes. **5.5** Add a `queue_peek` operation that does not remove an item. **5.6** Test that a failed pop leaves the output unchanged. **5.7** Encode `kind` and `count` in a documented byte order without copying the structure representation. **5.8** Compare the memory requirements of this queue and a linked queue for the same maximum capacity.


## 5.9 Structure layout as a local implementation detail

A structure value is a collection of logically related members, not a universally portable byte record. A compiler may insert padding before, between, and after members so that each can be accessed at an appropriate alignment. The expression `sizeof(struct sample)` therefore answers a question about this implementation's object layout; it does not define a network packet size.

Use `offsetof` from `<stddef.h>` when you genuinely need a member's offset within a structure. Avoid guessing offsets from the declaration. Embedded systems sometimes depend on a vendor-defined register structure layout, but that dependency should be explicit, reviewed against the compiler ABI, and separated from protocol serialization.

A structure assignment copies the member values, including pointer *values*, not heap allocations reachable through those pointers. If a record contains `char *name`, copying the record makes two references to the same storage. It does not perform a deep copy, extend the pointed-to object's lifetime, or establish a second independent owner.

## 5.10 Flexible array members

A final structure member can be a flexible array. It has no fixed element count in the structure's definition:

```c
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

struct message {
    size_t length;
    unsigned char bytes[];
};
```

A caller may allocate extra storage sufficient for the header and a requested number of byte elements. The size computation must be checked against `SIZE_MAX` before the call to `malloc`, and the allocated object must remain alive during access. This representation is useful for variable-length messages in one allocation, but it is inappropriate when a strictly bounded static buffer is preferable.

Flexible array members have restrictions: they belong at the end of an eligible structure, and the containing type cannot be used in every context where an ordinary fixed-size structure can. They also do not turn the member into a bounds-checked array; the actual allocation size still matters.

## 5.11 Layout-independent serialization

A portable encoder establishes each output byte deliberately. Consider a record with an 8-bit kind and 16-bit little-endian length. Its wire representation can use exactly three octets regardless of padding in the in-memory structure:

```c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool encode_header(unsigned char *dst, size_t size,
                   uint8_t kind, uint16_t length)
{
    if (dst == nullptr || size < 3) return false;
    dst[0] = kind;
    dst[1] = (unsigned char)length;
    dst[2] = (unsigned char)(length >> 8);
    return true;
}
```

This example assumes an octet-oriented destination and should be used only where the target's byte representation meets that contract. The field width and endianness are design decisions, independent of the host machine.

## 5.12 Data-structure choice follows requirements

A linked list offers insertion without moving contiguous records, but each node needs storage and a link, and traversal is not random access. A fixed array has locality and predictable overall capacity but may need to shift elements during insertion. A ring buffer is excellent for FIFO traffic but does not support arbitrary deletion efficiently.

For an MCU message queue with a known maximum occupancy, the fixed array or ring buffer may be easier to justify than a heap-backed list. If the queue is shared with an interrupt, however, a nonconcurrent ring implementation remains unsafe without a synchronization design. Capacity, ownership, timing, and concurrency are separate requirements.

**Further problems:** Implement a deep-copy operation for a record owning a dynamically allocated string. Show how to reject a flexible-array allocation whose size calculation overflows. Write an encoder and decoder for the three-octet header, testing both directions and malformed lengths.
