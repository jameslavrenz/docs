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


## 5.13 A structure is a value with invariants

A structure gives related fields one type, but the programmer decides which combinations of fields are meaningful. Imagine a temperature reading containing a numeric value, a unit identifier, and a validity flag. A declaration permits all combinations; the interface should say which represent actual measurements.

One design creates values through a constructor-like function that validates the inputs. An alternative stores an explicit status enumeration instead of a boolean flag so it can represent sensor-not-ready, disconnected, and measured states separately. The point is not to make every structure opaque; it is to choose a representation whose invariants are easy to state and preserve.

Designated initialization helps when a structure evolves because it names the intended members. A missing member is initialized according to the appropriate language rules, which may or may not express a valid application value. A status enumeration whose zero value has no defined meaning can produce a logically invalid initialized object despite being a perfectly valid C object.

## 5.14 Padding and equality

Two structures can have equal corresponding members while their padding bytes differ. Using `memcmp(&a, &b, sizeof a)` to test semantic equality is therefore not a general solution. Compare the meaningful fields instead:

```c
struct measurement { unsigned id; double value; };

bool same_measurement(struct measurement a, struct measurement b)
{
    return a.id == b.id && a.value == b.value;
}
```

The floating comparison here deliberately treats NaNs as unequal; whether this is appropriate depends on the application's data model. If bitwise reproducibility matters, define representation-specific behavior rather than assuming the members' `==` operators implement it.

For serialization, encode only documented fields in a documented order. Writing `sizeof(struct measurement)` bytes to a file can unintentionally expose padding bytes and platform-specific floating representations.

## 5.15 Tagged unions and exhaustiveness

A union can store one of several representations in shared storage. It does not automatically record which interpretation is active. A common approach pairs the union with a tag:

```c
enum value_kind { VALUE_INT, VALUE_FLOAT };
struct value {
    enum value_kind kind;
    union { int integer; double real; } data;
};
```

A function consuming `struct value` should examine the tag before accessing the corresponding member. C23 does not provide algebraic data types with exhaustive pattern matching; a `switch` over an enum can still have invalid values due to corrupted input or casts. An explicit default error path is useful at trust boundaries.

A tagged union is particularly effective for a parser returning one of several record kinds. It makes illegal reinterpretations easier to detect in code review and separates the logical type tag from the memory representation of the payload.

## 5.16 Intrusive versus owning lists

A linked-list node can own its payload, or the payload can contain the linkage needed to participate in the list. An intrusive list avoids a separate node allocation but ties the item's lifetime to list membership. An owning list may allocate a node and payload together or separately.

Before writing the first insert function, choose whether the list owns items. Can a caller free an item while it is linked? Can an item belong to two lists? What does removing an element do to its lifetime? These questions determine correctness more than the syntax of `next`.

On microcontrollers, a fixed pool of nodes and explicit occupancy flags may be preferable to unbounded heap allocation. It bounds memory, can eliminate allocation fragmentation, and makes exhaustion an ordinary status that tests can exercise.

## 5.17 Ring buffer wraparound

The earlier queue has capacity eight, with independent `head` and `count` fields. To find the tail, it computes `(head + count) % capacity`. This works because the queue invariant constrains `head` and `count`, and their sum is small enough to avoid overflowing `size_t`.

Trace a full cycle. Push values 0 through 7; the queue is full. A ninth push fails without modifying state. Pop 0, 1, and 2; now head is 3 and count is 5. Push 8, 9, and 10; the physical array slots reused are 0, 1, and 2. Subsequent pops return 3 through 10 in order.

The same logical sequence can be represented differently in physical storage. Clients should observe only the queue's documented behavior, not assume elements are contiguous in chronological order inside its array.

### Exercises

**5.9** Write an equality function for a structure containing an integer, a floating value, and an optional identifier; state the NaN policy. **5.10** Add a status tag to a variant record and reject an unsupported tag on decoding. **5.11** Design a fixed-capacity node pool for a list and describe release behavior. **5.12** Trace ring-buffer state after each push and pop, identifying array index and logical position.


## 5.18 A diagram of logical fields and physical layout

The declaration

```c
struct event {
    unsigned char kind;
    unsigned count;
    bool valid;
};
```

establishes three members with those types and in that order. It does not specify an identical packed sequence of their representations across every target.

An implementation might arrange them conceptually as follows:

```text
+------+---------+----------------+-------+-----------+
| kind | padding |     count      | valid | padding   |
+------+---------+----------------+-------+-----------+
```

The sizes and amount of padding in this sketch are deliberately unspecified. The compiler may choose a different layout to satisfy each member's alignment requirements. `offsetof(struct event, count)` can report the actual offset for the selected implementation, but using that result does not create a portable external file format.

The structure's *semantic* equality should compare members under a documented policy. Comparing `sizeof(struct event)` raw bytes with `memcmp` can read padding values that need not match even when corresponding members compare equal.

## 5.19 Representation is not serialization

A common mistake is to write a structure to disk with:

```c
fwrite(&event, sizeof event, 1, stream);
```

This can be acceptable for a tightly controlled, versioned, same-ABI internal checkpoint under a deliberately chosen contract, but it is generally unsuitable as a portable protocol or persistent format. The bytes may contain padding, depend on endianness, encode a compiler-specific `bool` representation, and change when the structure definition or ABI changes.

A portable message chooses field widths and byte order separately. For example, an event wire format might define one octet for kind, four little-endian octets for count, and one octet containing 0 or 1 for validity. Six octets are then required regardless of the native structure's `sizeof`, assuming eight-bit octets as required by the format.

A field encoder should write each octet deliberately. It should reject a count outside the format's chosen numerical range if the C source type could represent larger values, and should not read from or write to buffers before checking their declared bounds.

## 5.20 Padding does not solve ownership

Suppose a structure contains a pointer to allocated text:

```c
struct record {
    char *name;
    size_t length;
};
```

Assigning one `struct record` to another copies the pointer and length. It does not copy the characters. If both records now behave as independent owners, destroying each will attempt to free the same allocation.

A correct owning interface must define one of several policies. It can forbid copying, offer an explicit deep-copy operation, or transfer ownership from one record to another while clearing the old owner. C permits expressing each policy, but it does not automatically enforce it.

The same caution applies when records are moved inside a sorted array. Shifting a structure containing a pointer copies that pointer. If the data structure later destroys an overwritten record incorrectly, it can leak or double-release storage. A sorted array of fixed, inline word buffers avoids much of this complexity because each record carries its bytes directly.

## 5.21 Flexible array members, correctly sized

A flexible array member is a final array member with no declared bound. It permits one allocation to contain a header and a variable-length trailing sequence.

```c
struct message {
    size_t count;
    unsigned char payload[];
};
```

An allocation for `count` bytes should use a checked size calculation. The language permits padding differences between `sizeof(struct message)` and offsets reported for its flexible member, so a conservative size computation can be based on `sizeof(struct message)` plus the payload byte count, after checking that the sum fits in `size_t`.

```c
if (count > SIZE_MAX - sizeof(struct message))
    return nullptr;
struct message *m = malloc(sizeof(struct message) + count);
```

The snippet requires the appropriate headers and still must check `malloc` failure. The count may also need an application maximum smaller than `SIZE_MAX`. Because `payload` is an array of `unsigned char`, no multiplication is needed for its element size. A flexible array of larger element type would need both checked multiplication and checked addition.

There are important restrictions on structures with flexible array members. They are not a universal substitute for a vector type; the object still cannot grow by simply changing its `count` field. Resizing its allocation would require `realloc` or constructing another object, with the usual pointer invalidation and failure rules.

## 5.22 A tagged union as a small protocol model

A tagged union stores one of multiple logical value shapes:

```c
enum observation_kind { OBS_INTEGER, OBS_REAL };

struct observation {
    enum observation_kind kind;
    union {
        long integer;
        double real;
    } value;
};
```

The tag describes how the program should interpret the union. An input decoder must validate the tag before selecting the member, and it must reject unsupported values. C enums can hold values produced by casts or corrupt external data that do not correspond to any named enumerator.

A useful test constructs both valid variants, then constructs an invalid tag and confirms the consumer rejects it without reading an unintended member. This is a logical correctness test as well as a type-access safety discipline.

## 5.23 A ring buffer as a state machine

The fixed-capacity ring buffer in Chapter 5 can be described entirely through its state variables `head` and `count`. No pointer allocations are required.

The queue is empty when `count == 0` and full when `count == capacity`. A successful push stores at `(head + count) % capacity` and increments `count`. A successful pop reads `data[head]`, advances head modulo capacity, and decrements count.

To prove memory safety, assume initially that `head < capacity` and `count <= capacity`. Before a push, reject `count == capacity`; the calculated tail is a valid array index. Before a pop, reject `count == 0`; head is a valid index by invariant. Each update preserves the invariants.

The state-machine description does not establish concurrency safety. If an interrupt producer and task consumer modify the same `count` without synchronization, the program can have a data race. Chapter 16 develops a distinct SPSC design with atomic publication positions; the two queues are not interchangeable simply because both have circular arrays.

## 5.24 Representation tradeoffs for embedded firmware

Choose a representation using four criteria: maximum storage, worst-case operation cost, ownership complexity, and concurrency needs.

A fixed array has predictable allocation and simple indexed access. An intrusive list avoids external node allocations but requires careful membership and lifetime rules. A heap-backed list permits incremental growth but has per-node allocation costs and fragmentation concerns. A fixed pool provides bounded capacity and explicit slot reclamation. A ring buffer is appropriate for FIFO traffic and can be designed for specialized producer-consumer roles.

The best representation is not the one that uses the most advanced C syntax. It is the one whose invariants, resource bounds, and interfaces are easiest to verify for the application.

### Additional exercises

**5.13** Use `offsetof` to inspect a structure on two compilers without assuming the offsets are universal. **5.14** Write a deep-copy function for a record containing an owned string, including allocation-failure cleanup. **5.15** Derive the checked allocation size for a flexible array member of `uint32_t`. **5.16** Construct an invalid tagged-union value and define a safe error result. **5.17** Prove the ring buffer's `head` and `count` invariants after push and pop.


## 5.25 A fixed pool for bounded record ownership

An application can know its maximum number of live objects even when it does not know the exact times at which they will be created and released. A sensor interface might allow four outstanding requests. A telemetry task might buffer sixteen fixed-size events. A driver might have a small set of descriptors, each of which is either available or currently assigned to a client.

A fixed pool stores those objects in a statically bounded array and maintains availability metadata. It offers operations resembling allocation and deallocation without repeatedly calling `malloc` or `free`.

The following example uses four integer-valued slots, but the idea generalizes to records:

```c
enum { SLOT_CAPACITY = 4 };

struct slot {
    bool occupied;
    bool retired;
    unsigned generation;
    int value;
};

struct pool {
    struct slot slots[SLOT_CAPACITY];
};
```

Every `struct pool` owns its slots. Its storage lasts as long as the containing object. An automatic `struct pool` is destroyed when its lifetime ends; a static one lasts for the relevant program lifetime. Clients must not assume an acquired reference remains meaningful after its owning pool is destroyed.

A pool's maximum storage is fixed and can be inspected with `sizeof(struct pool)` on each target. Padding may make this larger than the sum of the visible field sizes. That matters in a small RAM budget but does not change the pool's logical capacity.

## 5.26 Why a raw index is not always enough

The simplest handle for a slot pool is an array index. A client receives index one, uses the record at index one, and eventually releases it. But the same index might later be reassigned to a different client.

Consider the sequence:

```text
Time    Slot 1               Client A              Client B
----    ------------------   --------------------  ----------------
t0      free                 —                     —
t1      owns value 10        receives index 1      —
t2      released             retains stale 1       —
t3      owns value 99        still has stale 1     receives index 1
t4      owns value 99        reads using index 1   —
```

At time `t4`, checking only that index one is occupied would allow A to read B's new logical object. The memory address is valid; the bug is *logical lifetime*, not necessarily an out-of-bounds access.

A generation-tagged handle adds a number that changes when the slot is released:

```c
struct handle {
    size_t index;
    unsigned generation;
};
```

The pool accepts a handle only if its index is valid, the slot is currently occupied, and the supplied generation matches the slot's current generation. A stale handle with an older generation is rejected.

The idea is analogous to versioning a resource identifier. It helps detect a stale reference even when the physical slot has been reused.

## 5.27 The pool's invariants

A pool operation can be reviewed using a small set of statements:

1. Every slot is either occupied or available. An occupied slot is not considered available to allocation.
2. A retired slot is never allocated again.
3. A successfully acquired handle identifies an occupied slot whose generation equals the handle's generation.
4. A successful release changes the slot's state so the old handle can no longer access it.
5. A failed acquire or release does not accidentally modify an unrelated slot.
6. Slot indexes are checked before array access.

The first allocation searches for an available, non-retired slot. Because `SLOT_CAPACITY` is a fixed small number, a linear scan is simple and takes a bounded number of slot examinations.

The implementation fills the selected slot and publishes the handle only after success. If no slot is available, it returns false and leaves the caller's out-parameter unchanged.

This is a useful *strong failure guarantee*. A caller can keep its previous handle after a failed request without wondering whether the pool partly updated it.

## 5.28 Stale handles and generation wraparound

Generation counters introduce their own corner case. If the counter is `unsigned`, incrementing it after `UINT_MAX` wraps to zero under C's unsigned arithmetic rules. A very old stale handle whose generation was zero might then match a newly assigned object after enough reuse cycles.

For teaching purposes, the companion `examples/slot_pool.c` avoids that possibility by retiring a slot when its current generation reaches `UINT_MAX`. A retired slot is never reused.

This policy exchanges eventual capacity loss for a simple lifetime argument: generations for a particular reusable slot never repeat. In most short-lived applications, exhaustion of a wide unsigned generation counter may be impractical, but a correctness proof should not rest on the assumption that an event is merely unlikely.

Other systems may choose much wider generation numbers, process-lifetime guarantees, or explicit epoch mechanisms. Their design must account for the length of time old handles can remain in circulation and for what happens after wraparound.

A generation number also cannot prevent every misuse. The teaching handle does not include the identity of its owning pool. If two different pool objects happen to have matching slot indexes and generation values, passing a handle from one pool to the other might be accepted. An API that must reject foreign-pool handles needs an additional identity mechanism or a type/interface boundary that prevents such cross-pool use.

## 5.29 No heap is not the same as no ownership

Although this pool performs no dynamic allocation, callers still have ownership obligations. One client must know when an acquired logical object can be released. Releasing it twice is invalid. Using the old handle after release is invalid. Destroying the pool while clients hold handles invalidates their logical references.

The storage's location does not eliminate those rules.

A pool also is not automatically thread-safe. Two concurrent clients searching for an available slot may both observe the same slot as free. Correct concurrent allocation requires synchronization or an algorithm specifically designed for multiple participants.

If an interrupt handler accesses the pool, the chosen synchronization mechanism must be appropriate for that context. A general mutex may not be callable from an ISR, and ordinary unsynchronized state updates may constitute data races in threaded C.

## 5.30 The practical design choices

A four-slot pool that scans at most four entries is easy to analyze. With thousands of entries, an explicit free list or bitmap may locate available slots more efficiently. Those representations introduce their own invariants and corruption hazards, so complexity should follow measured requirements.

A fixed pool is often appealing when:
- maximum simultaneous occupancy is known;
- allocation and release must be predictable;
- object sizes are uniform;
- heap fragmentation is undesirable;
- stale client handles should be detected.

It is less natural when individual items have very different sizes, when the maximum capacity is unknown, or when clients require independently resized objects.

The broader lesson is to choose a data structure around its observable contract: which resources are available, who owns them, when identities can be reused, and how failure is reported.

### Worked trace

Starting from an empty four-slot pool, acquire values 10, 20, 30, and 40. The pool is full. A fifth acquire fails and leaves the requested output handle unchanged.

Release the handle for slot one. Its generation increments. Acquire another record with value 99; slot one can be reused, but it now has a different generation. The original handle for slot one fails on lookup, while the new handle succeeds.

The standalone program tests this trace, double release, stale reads, and the retirement path at `UINT_MAX`.

### Exercises

**5.18** Add an explicit pool identifier to each handle and state how identifiers are kept unique during the pool's lifetime. **5.19** Add a `pool_write` operation that validates generation before updating the value. **5.20** Compare a scan-based pool with a bitmap-based pool for 64 fixed records. **5.21** Explain why permanent retirement avoids a generation wraparound alias but reduces capacity. **5.22** Design the synchronization required when multiple threads may acquire slots concurrently.
