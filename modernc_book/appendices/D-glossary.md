# Appendix D. Glossary of Modern C

A concise language benefits from precise terminology. The words below are used consistently throughout the manuscript.

**Abstract machine.** The language-defined model against which program behavior is judged. A compiler may transform code while preserving required observable behavior.

**Alignment.** A requirement constraining where an object of a particular type may be stored. Misaligned accesses can have undefined behavior.

**Array.** A contiguous sequence of elements of one element type. In many expression contexts an array converts to a pointer to its first element; this does not make arrays and pointers identical types.

**Automatic storage duration.** Storage normally associated with a block invocation and released when its lifetime ends.

**Binary interface (ABI).** Platform-specific conventions governing calling conventions, object layout and linkage. A C source declaration does not fix one universal ABI.

**Borrowed pointer.** A pointer whose use depends on an object owned or managed elsewhere. The pointer does not itself extend the object's lifetime.

**Buffer capacity.** The number of elements or bytes that an allocated or provided buffer may hold. Distinguish it from the number currently containing meaningful data.

**Callback.** A function supplied to another component for invocation under a documented protocol. Callbacks may need a context pointer and lifetime rules.

**Checked arithmetic.** An operation that detects whether a mathematical result fits the destination type before relying on its value.

**Compile-time assertion.** A translation-time constraint written with `static_assert` or a compatible form.

**Data race.** Conflicting access to a shared memory location by concurrent threads without the synchronization required by C's memory model, where at least one access modifies it and the accesses are non-atomic. Such a race has undefined behavior.

**Effective type.** Rules determining which lvalue types may be used to access stored objects. Relevant to aliasing optimizations and representation reinterpretation.

**Endianness.** The byte ordering used for a multi-byte representation. File formats and wire protocols must define their own byte ordering rather than assume the host's.

**Freestanding implementation.** An implementation environment, often embedded, with different entry-point and library obligations from a hosted environment.

**Half-open range.** An interval that includes its start and excludes its end, conventionally `[begin, end)`. This representation makes length equal to end minus begin when both positions belong to a valid common domain.

**Implementation-defined behavior.** Behavior for which the implementation must choose and document a permitted alternative.

**Invariant.** A property preserved across the operations of an algorithm or data structure. For a vector, `length <= capacity` is fundamental.

**Lifetime.** The period during which an object exists. A pointer can retain an old bit pattern after its target object's lifetime has ended, but that does not make dereferencing it valid.

**Lvalue.** An expression designating an object or function under the rules of C. Not every lvalue is modifiable.

**Object representation.** The bytes of an object's stored representation. Copying them does not automatically define a portable external serialization format.

**Opaque type.** A type whose representation is hidden from the client, commonly through an incomplete structure declaration and operations accepting pointers.

**Ownership.** An application-level convention identifying which component is responsible for releasing a resource. C does not enforce a unique-owner discipline automatically.

**Precondition.** A condition the caller must satisfy before calling an operation, such as providing a valid pointer and sufficient length.

**Postcondition.** A property guaranteed when an operation finishes under its documented conditions, including what happens on failure.

**Release/acquire synchronization.** Atomic memory ordering used to publish data and establish visibility of earlier writes to a reader that observes the publication.

**Restrict.** A pointer qualification expressing an access promise enabling optimization when its conditions are respected; not a runtime alias check.

**Sentinel.** A distinguished value such as `EOF` indicating a condition outside the normal result domain.

**Signed overflow.** Evaluating a signed arithmetic result not representable in the result type. This remains undefined behavior in C23.

**Static storage duration.** Storage that persists for program execution, regardless of whether the identifier has file scope or block scope.

**Stream.** C's hosted abstraction for a sequence of input or output characters or bytes, often associated with a terminal or file.

**Translation unit.** The result of preprocessing one source file, including its headers, that is compiled as a unit.

**Undefined behavior.** A program action for which the standard imposes no requirements. An observed result in one build provides no portable contract.

**Unspecified behavior.** Behavior for which the standard allows more than one alternative without requiring the implementation to document its choice.

**Volatile.** A qualifier affecting accesses with observable side effects. It is not a general replacement for atomics or mutexes.

This glossary is deliberately selective. The ISO standard supplies precise normative definitions, sometimes with narrower conditions than a one-sentence teaching description can express.
