# Preface

C is a compact language, but mastering it is not the same as memorizing a compact set of keywords. The interesting questions begin where a program meets actual storage, input, timing, or another component: Who owns this buffer? How large is it? What happens when conversion overflows? Which operations are portable? What can another thread observe? A useful modern introduction should make those questions part of ordinary programming practice.

This book presents C23 through small programs and progressively more demanding examples. The chapters begin with stream processing and basic expressions, proceed to functions, arrays, structures, memory allocation and compilation, and then develop the correctness concerns that matter in systems software. Later chapters explore C23 additions, binary protocols, concurrency, callbacks, text encodings, and testing.

It is intentionally an **independent original textbook**. Its concise presentation and example-first progression acknowledge the value of classic C pedagogy, but this is not a third edition of *The C Programming Language* and is not affiliated with its authors or publisher. The examples, explanations, and exercises here are newly written.

## Audience

The intended reader has some programming experience but need not have used C professionally. A reader comfortable with another language can move quickly through the opening material. A reader new to pointers and manual storage management should slow down, compile every complete example, and work through the boundary-case exercises.

The later chapters are especially relevant to systems and embedded engineers. They discuss host-versus-target portability, fixed-capacity data structures, interrupt-related limitations, and memory-ordering contracts. They do not assume that every MCU provides the full hosted C library, filesystem, heap, or threads.

## What modern means

C23 is the 2024 publication of ISO/IEC 9899:2024, often described as the 2023 revision. Modern C encompasses both the updated language and a more explicit engineering discipline: prototypes rather than implicit declarations, documented ownership, checked sizes, controlled resource lifetimes, clean module interfaces, and tests that challenge failure paths.

Some C23 features have uneven implementation support. A compiler can recognize `-std=c23` without supplying every related library header or freestanding runtime facility. This manuscript therefore distinguishes language rules, implementation-defined properties, and deployment-specific services. It uses modern constructs where they clarify intent, but does not pretend one compiler flag can make every old target conform completely.

## How to use the examples

Listings fall into two categories. A **complete program** has its own translation unit and entry point and may be compiled and run. An **illustrative fragment** shows a specific language rule or API contract and may omit enclosing declarations, headers, and supporting functions. The standalone programs are kept in the repository's `examples/` directory; the Makefile and test scripts document how they are built.

A useful study cycle is to predict the program's result, compile it with warnings enabled, run it on normal inputs, and then design adversarial boundary cases. For pointer-heavy code, write down the pointed-to object's owner and lifetime. For arithmetic, identify intermediate types and range limits before evaluation. For input processing, state whether a malformed record is rejected, partially accepted, or reported as incomplete.

Tests are evidence rather than proofs of universal correctness. A green hosted CI run is valuable, but cannot replace device-level validation of registers, DMA, interrupts, scheduling, and target libraries. Where a complete listing is not independently compile-tested, the manuscript should not claim otherwise.

## Conventions

Code uses C23 spelling such as `bool`, `nullptr`, and empty initialization where appropriate. System-specific code is labeled as such. A pointer and length together describe a counted region only when the caller actually provides storage of that length; no signature can verify an arbitrary dangling pointer. An API may require a writable out-parameter and promise to leave it unchanged on failure. Such contracts are explained when introduced and reused consistently.

Array lengths and capacities are measured in elements unless explicitly described as bytes. Byte-oriented protocols specify octet widths when needed. Integer-overflow checks occur before potentially overflowing arithmetic. Sample code favors clarity and verifiable bounds over clever expression tricks.

## An author's checklist for each chapter

A complete treatment should answer four questions. First, what problem does the feature solve? Second, what does the C language actually guarantee? Third, what assumptions does the example make about its caller or platform? Fourth, how would an engineer test the normal path and its boundaries? This revision was expanded with those questions in mind.

The manuscript remains a draft while technical review, compiler verification, layout checking, and indexing continue. The intended final length is roughly 200–230 typeset pages, but page count alone is not a substitute for coherent explanation.

## Reading path

Chapters 1–8 establish the everyday language and program structure. Chapters 9–12 connect the modern standard to reliability and a larger example. Chapters 13–19 deepen testing, representation, semantics, concurrency, callbacks, text, and the preprocessor. The appendices are designed to be consulted while writing programs rather than read in one uninterrupted pass.

If a reader remembers one principle, it should be this: in C, **the contract is part of the program**. A short function is not complete until its types, bounds, ownership, and failure behavior are understood.
