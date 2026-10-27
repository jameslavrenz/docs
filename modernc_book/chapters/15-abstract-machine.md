# 15. Expressions, Objects, and the Abstract Machine

A C program is not defined by the machine instructions observed in one build. It is defined by the language's abstract machine: a model of evaluation, objects, values, and observable behavior. A compiler is free to transform an implementation as long as it preserves the behavior required of a conforming program.

This principle explains both C's performance and its hazards. A short expression can have a precise meaning, an implementation-dependent meaning, or no defined meaning at all.

## 15.1 Values and objects

An *object* is a region of data storage whose contents can represent values. An *expression* can produce a value, designate an object, or have side effects. Consider:

```c
int number = 7;
int *address = &number;
*address = 9;
```

The pointer contains a value that designates `number`. The indirection expression `*address` is an lvalue; the assignment modifies the designated object. The validity of the operation depends on `number` being alive, accessible, and represented by a suitable pointer.

Not every expression is a modifiable lvalue. A constant expression, a nonmodifiable qualified lvalue, and an array expression cannot simply be treated as destinations for assignment.

## 15.2 Storage duration and scope answer different questions

*Scope* describes where an identifier may be used in source code. *Storage duration* describes the lifetime of storage associated with an object. A local `static` object has block scope but static storage duration:

```c
unsigned next_identifier(void)
{
    static unsigned value = 0;
    return ++value;
}
```

The object persists across calls. The function has a new problem: unsigned wraparound eventually repeats an identifier. It is also not inherently safe for unsynchronized concurrent calls. A compact interface does not guarantee suitable semantics.

An automatic object, by contrast, usually exists only for the relevant invocation. Returning its address does not extend its lifetime.

## 15.3 Sequenced and unsequenced evaluations

Consider the expression

```c
int result = f() + g();
```

The language does not require the call to `f` to finish before the call to `g`, or vice versa. It does require both results before their addition can produce the final result. If the functions only compute independent values, that freedom usually does not matter.

If both calls modify the same non-atomic scalar object, their unsequenced side effects can make the expression undefined. Write separate statements when an ordering is required:

```c
int left = f();
int right = g();
int result = left + right;
```

The full expressions are sequenced in source order. Still check that the addition itself is representable for signed types.

The operators `&&`, `||`, and comma (when used as an operator) introduce sequencing properties that differ from ordinary arithmetic operators. A function-call argument list is not an ordering guarantee.

## 15.4 Integer overflow and conversions

For an unsigned integer type, arithmetic is performed modulo one more than its maximum representable value. Signed arithmetic overflow has undefined behavior. These are not implementation details that can safely be ignored because a processor uses two's complement.

```c
#include <limits.h>
#include <stdbool.h>

bool increment_int(int value, int *out)
{
    if (out == nullptr || value == INT_MAX)
        return false;
    *out = value + 1;
    return true;
}
```

The guard precedes the addition. A post-addition check would already be too late.

Conversion introduces another boundary. Narrowing an integer to a smaller unsigned type follows modular rules. Converting an out-of-range value to a signed integer type has implementation-defined behavior or can raise an implementation-defined signal. Prefer range checks before narrowing, particularly when parsing files or messages.

## 15.5 Effective type and aliasing

Object access rules restrict which lvalue types may be used to read or write stored objects. A programmer cannot generally take the address of a `float`, cast it to `int *`, dereference it, and expect portable reinterpretation. Doing so can violate alignment and effective-type rules as well as make code sensitive to representation.

For object-representation inspection, an appropriately sized array of `unsigned char` and `memcpy` are usually clearer. Representation inspection does not imply that every bit pattern is a valid value of every destination type.

```c
#include <string.h>
#include <stdint.h>

uint32_t bits_of_float(float x)
{
    uint32_t bits = 0;
    /* Requires equal sizes, verified for the selected target. */
    if (sizeof bits == sizeof x)
        memcpy(&bits, &x, sizeof bits);
    return bits;
}
```

This example exposes representation bytes without violating aliasing rules, but the resulting integer's meaning depends on the target's floating-point representation and byte order. It is **not** a portable way to serialize a floating value.

## 15.6 Order, lifetime, and optimizers

A valid optimizer may assume that undefined behavior does not occur. Consequently, source checks appearing after invalid operations may be removed. The correct style is to validate the condition *before* executing the operation it protects.

For array access, verify the index against a known count before evaluation of `array[index]`. For allocation, verify multiplication before computing required bytes. For pointer use, ensure the object has not been freed or otherwise had its lifetime ended.

## 15.7 A small checked interval

Suppose a program writes `count` bytes beginning at `offset` into a buffer of `capacity` bytes. The tempting check `offset + count <= capacity` can itself overflow when the quantities use an unsigned type. Rewrite the requirement into conditions that can be checked safely:

```c
#include <stdbool.h>
#include <stddef.h>

bool fits(size_t offset, size_t count, size_t capacity)
{
    return offset <= capacity && count <= capacity - offset;
}
```

The `&&` operator short-circuits. Therefore `capacity - offset` is evaluated only when `offset <= capacity`. This both avoids underflow and expresses the intended invariant.

A complete executable version with boundary tests appears in `examples/checked_interval.c`.

## 15.8 Exercises

**15.1** Explain why `i = i++ + 1` does not provide a portable way to increment a scalar.

**15.2** Give an example of block scope combined with static storage duration. Describe the consequences for recursive calls.

**15.3** Implement a checked multiplication of two `size_t` values, preserving its output argument on failure.

**15.4** Design a test matrix for `fits` including zero capacity, exact fits, large offsets, and `SIZE_MAX`.

**15.5** Explain why reading an integer directly from an arbitrary byte-buffer address can violate alignment or effective-type rules.

**15.6** Distinguish a language's unspecified evaluation order from an implementation-defined integer conversion.


## 15.9 The optimizer reasons from the language rules

Compilers are not required to preserve an apparent intention that depends on undefined behavior. Consider:

```c
int *p = /* supplied by caller */;
int value = *p;
if (p == nullptr)
    return -1;
```

The program already dereferences `p` before testing it. A compiler may optimize under the assumption that the dereference had defined behavior; moving the test after the access has made it ineffective. The correct ordering is to check the pointer first and then dereference.

Likewise, `int sum = a + b; if (sum < a) ...` does not provide a general overflow test for signed integers, since overflowing addition already has undefined behavior. Establish representability before evaluating the addition.

## 15.10 Object representation versus value

Every object has an object representation consisting of bytes. The interpretation of those bytes depends on the type. Some object representations can contain padding or trap representations for some types or implementations. Reading arbitrary bytes as if they were a fully initialized object of any chosen type is not generally safe.

The character types have special significance for inspecting object representations. Copying bytes using `memcpy` is commonly preferable to violating aliasing rules through an incompatible pointer cast. But reproducing the bytes of a pointer or floating-point value on another machine does not guarantee the same value there.

## 15.11 Sequence and indirection

An expression such as `array[i++] = array[i]` deserves suspicion: side effects and value computations may be unsequenced in a way that violates the rules. A clearer version splits responsibilities:

```c
size_t index = i;
int value = array[index];
array[index] = value;
++i;
```

This example is intentionally trivial, but each step now has an explicit order. The larger lesson is not to ban compact expressions, but to separate operations when sequencing is a necessary part of correctness.

## 15.12 Bounds and pointer provenance

A pointer is more than a numeric address for the purposes of the C abstract machine. Pointer arithmetic and dereference are constrained by the object and its lifetime. Converting an arbitrary integer to a pointer and dereferencing it is not portable ISO C, even on targets where hardware maps devices at fixed numeric addresses.

Freestanding systems often provide implementation-specific extensions for memory-mapped I/O and linker-defined memory locations. Such code should be isolated and documented rather than presented as a portable language feature.

## 15.13 A reasoning template for code reviews

For each expression ask: Which types participate? What conversions occur? Which operations can overflow or have invalid operands? What is the evaluation ordering? Which object is accessed? Is its lifetime active, and is the access permitted through this lvalue type?

This short set of questions is particularly effective for parsing, arithmetic kernels, and pointer-heavy utilities.

**Further problems:** Rewrite an overflow-after-addition test into a valid precondition check. Identify the violated rule in a dangling-pointer return. Explain why `memcpy` may safely move representation bytes even though a cast-and-dereference technique is not generally interchangeable.

## 15.14 Draw the memory model before optimizing

A useful sketch separates objects and pointers. For a vector, draw one structure containing a pointer, a length, and a capacity. Draw a separate heap block containing the elements. A shallow assignment of the vector copies its three fields into another structure, creating another pointer to the same block. It does not allocate a second block.

Now suppose one vector calls `realloc`. The old block's lifetime ends on success. Any pointer borrowed from the old block is invalid; the second shallow-copied owner also has an invalid pointer. Destroying both structures can produce invalid releases. The simplest fix is not a clever cast or compiler option but an ownership rule forbidding such copies.

## 15.15 Distinguish value equality from byte equality

For an ordinary `struct`, two values may have equal members but different padding bytes. Comparing their raw representations with `memcmp` is not a general semantic equality test. For floating values, even member-wise `==` needs an application policy concerning NaNs and signed zeros.

Serialization is a further step: it needs a stable external format independent of structure padding, compiler ABI, and machine byte order. This is why Chapter 14 constructs protocol fields byte-by-byte instead of writing the in-memory structure directly.

## 15.16 Expression transformations need preconditions

A mathematically valid rearrangement is not always valid for finite-width C arithmetic. The expression `offset + count <= capacity` looks equivalent to `count <= capacity - offset`, but only the latter can be evaluated safely after first checking `offset <= capacity` when all quantities have type `size_t`.

Similarly, `a + b` can overflow for signed integers even when a later comparison attempts to detect it. A valid transformation must preserve the language-defined behavior for all permitted inputs, not merely produce the same result on small examples.

## 15.17 Observable behavior and the optimizer

Compilers may reorder and eliminate operations when permitted by the language's abstract-machine rules. The fact that a debugger shows a particular sequence of machine instructions does not make that sequence the semantics of the source program.

For concurrent code, atomics and synchronization express ordering requirements across threads. `volatile` affects certain observable accesses but is not a general mechanism for sharing ordinary variables safely. For device registers, additional target-specific rules determine bus transactions and barriers.

### Exercises

**15.7** Draw the heap and stack objects involved in `vector_append`, then redraw them after successful growth. **15.8** Identify when pointer aliases become invalid. **15.9** Compare two structures semantically without reading padding bytes. **15.10** Transform a potentially overflowing length sum into a checked subtraction formulation.
