# Appendix F. Topic Finder

This topic finder is organized alphabetically for readers using the PDF or a printed draft. References name chapters rather than final page numbers, because pagination will change during the remaining editorial work. Subtopics are included when closely related concepts appear in different chapters.

For the precise normative meaning of any language rule, consult ISO/IEC 9899:2024. This index is a navigation aid, not an alternative language specification.

### A

- **Abstract machine:** expression semantics, observable behavior, and optimization, Chapter 15.
- **Acquire ordering:** one-shot publication and SPSC queues, Chapter 16.
- **Alignment:** pointer access restrictions, Chapter 15; arena storage, Chapter 7; protocol decoding, Chapter 14.
- **Allocation:** `malloc`, `calloc`, `realloc`, and ownership, Chapter 7; resource failure tests, Chapter 13.
- **Arena allocator:** fixed-capacity bytes, reset semantics, and alignment limits, Chapter 7; worked solution, Appendix C.
- **Array decay:** array-to-pointer conversion and function parameters, Chapters 3–4.
- **Array indexing:** bounds, one-past pointers, counted slices, Chapter 4.
- **ASCII:** digit parsing and protocol tokens, Chapters 2, 3, and 18.
- **Atomics:** data races, ordering and lock-free limitations, Chapter 16.
- **Automatic storage duration:** object lifetimes, Chapters 3 and 15.

### B

- **Binary literals:** C23 digit syntax, Chapter 9.
- **Binary protocols:** byte layouts and endianness, Chapters 10 and 14.
- **Bit-precise integers:** `_BitInt` and ranges, Chapter 9.
- **Bit rotation:** unsigned shifts, count bounds and tests, Chapter 14.
- **Bitwise operators:** masks, shifts, representation issues, Chapter 14.
- **Boolean values:** `bool` and C23 keyword spelling, Chapter 9.
- **Borrowed pointers:** views, lifetimes and ownership, Chapters 4 and 7.
- **Bounded input:** line reader, Chapter 6; TLV and frame decoders, Chapter 10; UTF-8 streaming, Chapter 18.
- **Buffer capacity:** slices, vectors, arenas, Chapters 4 and 7.

### C

- **Callbacks:** function pointers and context, Chapter 17.
- **Character encodings:** UTF-8 and character count, Chapter 18.
- **Checked arithmetic:** addition, subtraction and multiplication, Chapters 2, 9 and 15; Appendix B.
- **CI:** compiler matrix, regression suites and reproducibility, Chapters 8 and 13.
- **Comma operator:** sequencing and expression rules, Chapter 15.
- **Comparators:** `qsort`, signed overflow and consistent ordering, Chapter 17.
- **Compound literals:** initialization and object lifetimes, Chapters 2 and 15.
- **`const` qualification:** pointer access and contracts, Chapter 4.
- **`constexpr`:** C23 constant objects and feature probes, Chapter 9.
- **Conversions:** promotions, narrowing, signed/unsigned arithmetic, Chapter 2.
- **Copying storage:** `memcpy` versus `memmove`, Chapter 4.
- **C23 migration:** language changes and toolchain probes, Chapter 9; Appendix E.
- **CRC and checksums:** format checks and limitations, Chapters 10 and 14.

### D

- **Decoder, streaming frame:** four-state parser, bounded work and resynchronization, Chapter 10; worked solution, Appendix C.
- **Data races:** non-atomic shared objects and undefined behavior, Chapter 16.
- **Declarations:** pointers, functions, arrays and prototypes, Chapters 3–4; Appendix A.
- **Decoding:** counted input, TLV formats, UTF-8 state machines, Chapters 10 and 18.
- **Designated initialization:** structures, Chapters 5 and 9.
- **Device registers:** volatile access and hardware semantics, Chapter 14.
- **Diagnostics:** compiler warnings and sanitizers, Chapter 13.
- **Digit parsing:** unsigned and signed decimal boundaries, Chapters 2–3; Appendix C.
- **Dynamic memory:** ownership and allocation failures, Chapter 7.

### E

- **Effective type:** aliasing and legal object access, Chapter 15.
- **Embedding resources:** `#embed` and toolchain support, Chapters 9 and 19.
- **Empty initialization:** C23 `{}`, Chapters 5 and 9.
- **Encoding:** UTF-8 and binary field formats, Chapters 14 and 18.
- **Endianness:** multi-byte values in external formats, Chapter 14.
- **EOF:** stream termination and incomplete records, Chapters 1 and 6.
- **Error statuses:** parser and I/O contracts, Chapters 3, 6 and 10.
- **Exercise solutions:** Appendix C.

### F

- **Fixed slot pool:** generational handles, retirement and stale references, Chapters 5 and 7; worked solution, Appendix C.
- **Failure injection:** allocators and byte sinks, Chapters 7 and 13; Appendix C.
- **File I/O:** `FILE *`, buffering, short reads and writes, Chapter 6.
- **Flexible array members:** variable-length records, Chapter 5.
- **Floating point:** accuracy, range and NaNs, Chapter 2.
- **Frame decoder:** byte layouts, checksum and strong failure safety, Chapters 10 and 14.
- **Freestanding C:** library availability and MCU toolchains, Chapters 8, 9 and 16.
- **Function pointers:** callbacks and generic interfaces, Chapter 17.
- **Functions:** prototypes, out-parameters and linkage, Chapter 3.

### G–H

- **`getchar`:** the `int` result and `EOF`, Chapter 1.
- **Generic selection:** `_Generic`, Chapter 19.
- **Headers:** declarations, include guards, standalone inclusion, Chapters 3 and 8.
- **Heap allocation:** `malloc`, `realloc`, capacity planning, Chapter 7.
- **Hosted implementations:** entry points, files and threads, Chapters 1, 6 and 16.

### I–L

- **Implementation-defined behavior:** representation and portability, Chapters 2, 9, 15; Appendix A.
- **Invariants:** loop proofs, buffers, containers, Chapters 4, 7, 10 and 11.
- **Integer conversion:** representability before casts, Chapter 2.
- **Integer overflow:** signed undefined behavior, unsigned modular operations, Chapters 2 and 15.
- **Interrupts:** RTOS and atomic restrictions, Chapter 16.
- **I/O errors:** short transfers, buffering and cleanup, Chapter 6.
- **Lifetime:** automatic, static and allocated objects, Chapters 7 and 15.
- **Line input:** bounded line reader and resynchronization, Chapter 6.
- **Linkage:** translation units, internal and external definitions, Chapters 3 and 8.
- **Lock-free algorithms:** progress properties, memory order and hardware constraints, Chapter 16.

### M–N

- **Macros:** double evaluation, stringizing and token pasting, Chapter 19.
- **Memory layout:** arrays and pointers, Chapter 4; structures and padding, Chapter 5.
- **Memory ordering:** relaxed, release, acquire and sequential consistency, Chapter 16.
- **`memcpy`:** copying object representations, Chapters 4 and 15.
- **`memmove`:** overlapping buffers, Chapter 4.
- **Modules:** header plus implementation, Chapters 3 and 8.
- **Multi-threading:** C threads and resource cleanup, Chapter 16.
- **NaN:** floating equality and comparator behavior, Chapters 2, 5 and 17.
- **Narrowing conversions:** checked integer ranges, Chapter 2.
- **`nullptr`:** C23 null pointer constant, Chapter 9.
- **Numeric parsing:** grammar and overflow, Chapters 2, 3 and 18.

### O–P

- **Object representation:** padding, serialization and aliasing, Chapters 5, 14 and 15.
- **One-past pointer:** valid construction versus invalid dereference, Chapter 4.
- **Opaque interfaces:** callback contexts and ownership, Chapters 3 and 17.
- **Ordering, evaluation:** function arguments and side effects, Chapters 3 and 15.
- **Obsolete handles:** logical lifetime versus physical storage, Chapters 5, 7 and 15.
- **Ownership:** borrowed and owning references, Chapters 4 and 7.
- **Padding:** structure size and equality, Chapter 5.
- **Partial writes:** callbacks and I/O, Chapters 6 and 17.
- **Pointer arithmetic:** valid arrays and bounds, Chapter 4.
- **Pointer provenance and lifetime:** object access restrictions, Chapter 15.
- **Portable serialization:** bytes versus native structures, Chapter 14.
- **Preconditions:** valid pointer ranges and object lifetime, Chapters 3 and 10.
- **Preprocessor:** inclusion, conditionals and macros, Chapter 19.

### Q–R

- **`qsort`:** comparator contract and overflow-safe comparisons, Chapter 17.
- **`realloc`:** failure guarantees, invalidation and C23 zero-size behavior, Chapter 7.
- **Recursion:** base cases, limits and task stack use, Chapter 3.
- **Release ordering:** producer publication, Chapter 16.
- **Representation:** types, structure layout and wire formats, Chapters 5 and 14.
- **Resource cleanup:** partial acquisition and release, Chapters 3 and 7.
- **`restrict`:** aliasing promises and optimization, Appendix A.
- **Ring buffers:** fixed-capacity FIFO, Chapter 5; atomic SPSC, Chapter 16.
- **RTOS:** thread and interrupt portability, Chapters 8 and 16.

### S

- **State-transition testing:** parser and pool validation, Chapter 13.
- **Sanitizers:** testing evidence and limitations, Chapter 13.
- **Sequencing:** unsequenced effects and evaluation ordering, Chapter 15.
- **Serialization:** field widths, endianness, padding, Chapters 5 and 14.
- **Signed integers:** C23 representation, conversion and overflow, Chapter 2.
- **Slices:** pointer/length views and range proofs, Chapter 4.
- **SPSC queue:** atomic producer-consumer exchange, Chapter 16.
- **`static`:** storage duration and internal linkage, Chapters 3 and 15.
- **`static_assert`:** translation-time assertions, Chapters 9 and 19.
- **Storage duration:** object lifetime, Chapter 15.
- **Streams:** text and binary modes, error checks, Chapter 6.
- **String literals:** static storage and mutability, Chapter 4.
- **`strtol`:** parser grammar, end pointers and range errors, Chapter 6.
- **Structures:** fields, padding and ownership, Chapter 5.

### T–U

- **TLV format:** bounded type-length-value decoding, Chapter 10.
- **Thread lifecycle:** creation, partial failure and joining, Chapter 16.
- **Time bounds:** bounded memory versus bounded execution, Chapters 6, 10 and 16.
- **Toolchains:** host versus cross-compiler, Chapter 8; feature support, Chapter 9.
- **Translation units:** source/header compilation and linking, Chapter 8.
- **Type inference:** C23 `auto` and `typeof`, Chapter 9.
- **Undefined behavior:** overflow, invalid pointers, data races, Chapter 15.
- **Unicode:** scalar values, UTF-8 and normalization boundaries, Chapter 18.
- **Unspecified behavior:** evaluation order and portability, Chapter 15.
- **Unsigned arithmetic:** modular rules and checked sizes, Chapter 2.
- **UTF-8:** complete and incremental decoders, Chapter 18.

### V–Z

- **`volatile`:** device access versus thread synchronization, Chapters 14 and 16.
- **Vector:** dynamic capacity, ownership, and failure guarantees, Chapter 7.
- **Warnings:** pedantic diagnostics and compiler differences, Chapters 9 and 13.
- **Wait-free algorithms:** progress guarantees, Chapter 16.
- **Word index:** sorting, insertion invariants, errors and complexity, Chapter 11.
- **Zero initialization:** aggregate initialization versus raw bytes, Chapters 5, 7 and 9.
- **Zero-length requests:** allocation and slice policies, Chapters 4 and 7.

The final typeset edition should replace or supplement these chapter-level entries with verified page references. Automated PDF pagination is not stable enough during manuscript expansion to maintain a conventional fixed-page index by hand.
