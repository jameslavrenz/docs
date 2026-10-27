# Modern C: A Concise Introduction to C23

**An independent textbook** · Draft 0.1 · 26 October 2026

An original, example-first introduction to the C programming language as standardized by ISO/IEC 9899:2024 (C23). Its compact teaching progression is inspired by the general *pedagogical approach* of classic concise programming texts. This is **not** a third edition of *The C Programming Language*, and is not written by or affiliated with Brian Kernighan, Dennis Ritchie, or their publishers.

## Reader's route

1. [A First Look at C](chapters/01-first-look.md)
2. [Types, Expressions, and Control](chapters/02-types-expressions.md)
3. [Functions and Program Structure](chapters/03-functions.md)
4. [Pointers, Arrays, and Strings](chapters/04-pointers-arrays.md)
5. [Structures and Data Representation](chapters/05-structures.md)
6. [Input, Output, and Files](chapters/06-io.md)
7. [Dynamic Memory and Ownership](chapters/07-memory.md)
8. [The Preprocessor and Separate Compilation](chapters/08-build.md)
9. [The C23 Language](chapters/09-c23.md)
10. [Reliable Systems Programming](chapters/10-reliability.md)
11. [An Extended Example: Word Index](chapters/11-word-index.md)
12. [Exercises and Further Reading](chapters/12-exercises.md)

## Build and scope

Use a C23-capable compiler, e.g. `gcc -std=c23 -Wall -Wextra -Wpedantic -O2 program.c -o program`. GCC 15 and newer select C23 by default; Clang 18 and newer recognize `-std=c23`. Individual standard-library facilities can lag behind language support. Check the specific toolchain and C library.

The intended finished book is roughly **200–230 typeset pages**, comparable in scale to the first edition of the classic text. This repository currently contains an **initial, deliberately shorter manuscript**, not a claim that the final page target has already been met. Content is written in original language; exercises emphasize reasoning and building small programs.

## Sources for language conformance

- [ISO C23 overview](https://en.cppreference.com/w/c/23)
- [GCC C language standards status](https://gcc.gnu.org/projects/c-status.html)
- [Clang C language status](https://clang.llvm.org/c_status.html)

License: All original prose and examples © 2026; rights reserved pending a separately selected license.
