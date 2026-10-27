# Modern C: A Concise Introduction to C23

**An independent textbook** · Expanded technical-review draft · 26 October 2026

An original, example-first introduction to the C programming language as standardized by ISO/IEC 9899:2024 (C23). Its compact teaching progression is inspired by the general *pedagogical approach* of classic concise programming texts. This is **not** a third edition of *The C Programming Language*, and is not written by or affiliated with Brian Kernighan, Dennis Ritchie, or their publishers.

## Reader's route

[Preface: Audience, conventions, and approach](frontmatter/preface.md)

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
13. [Testing, Diagnostics, and Portability](chapters/13-testing.md)
14. [Bits, Bytes, and Portable Data](chapters/14-bitwise-portable-data.md)
15. [Expressions, Objects, and the Abstract Machine](chapters/15-abstract-machine.md)
16. [Atomics, Threads, and Communication](chapters/16-atomics-concurrency.md)
17. [Function Pointers, Callbacks, and Interfaces](chapters/17-callbacks.md)
18. [Text, Encodings, and Robust Parsers](chapters/18-text-encodings.md)
19. [Compile-Time Programming and the Preprocessor](chapters/19-compile-time.md)

### Appendices

- [A. Working Reference to C23](appendices/A-language-reference.md)
- [B. Standard Library by Task](appendices/B-standard-library.md)
- [C. Selected Worked Exercises](appendices/C-exercise-solutions.md)
- [D. Glossary of Modern C](appendices/D-glossary.md)
- [E. Conformance and Migration Checklist](appendices/E-conformance.md)
- [F. Alphabetical Topic Finder](appendices/F-topic-finder.md)
- [Compilable example files](examples/README.md)

All nineteen chapters received a second-pass technical expansion: more explanation of preconditions, numeric boundaries, lifetime, ownership, explicit byte formats, concurrency, and C23 portability. Six appendices supply reference tables, selected solutions, a glossary, and a deployment checklist.

## Newly expanded worked examples

The current expansion adds detailed treatments of module boundaries, sorted word indexing, packet validation, failure injection, and callback-driven output. The matching complete examples are:

- [`frame_codec.c`](examples/frame_codec.c): checked byte-frame encoding and decoding with strong failure behavior.
- [`callback_sink.c`](examples/callback_sink.c): bounded partial-write handling through a callback and fake output device.
- [`modules/`](examples/modules/): a separately compiled interface with a self-contained header and executable client.

Further examples include [`arena_bytes.c`](examples/arena_bytes.c) (bounded byte arena with reset semantics), [`parse_signed.c`](examples/parse_signed.c) (checked decimal parsing including `LONG_MIN`), and [`rotate_bits.c`](examples/rotate_bits.c) (defined 32-bit rotations). The latest systems examples are [`slot_pool.c`](examples/slot_pool.c), a fixed-capacity pool with generation-tagged handles, and [`frame_stream.c`](examples/frame_stream.c), a byte-at-a-time framed-message decoder. Further streaming and binary-format examples include [`utf8_stream.c`](examples/utf8_stream.c), an incremental strict UTF-8 decoder, and [`tlv_decode.c`](examples/tlv_decode.c), a bounded TLV record decoder. The newer I/O and threading examples are [`line_reader.c`](examples/line_reader.c), a bounded hosted line reader with overlong-record recovery, and [`thread_counter.c`](examples/thread_counter.c), a two-worker C11/C23 atomic-counter demonstration using the hosted threads API. These supplement the counted-slice and checked-vector examples. Run `make check` to compile and execute all 33 standalone test programs on a C23-capable toolchain.

## Build and scope

Use a C23-capable compiler, e.g. `gcc -std=c23 -Wall -Wextra -Wpedantic -O2 program.c -o program`. GCC 15 and newer select C23 by default; Clang 18 and newer recognize `-std=c23`. Individual standard-library facilities can lag behind language support. Check the specific toolchain and C library.

The intended finished book is roughly **200–230 typeset pages**, comparable in scale to the first edition of the classic text. This repository currently contains an **expanded draft, not yet established to meet the target page count**, not a claim that the final page target has already been met. Content is written in original language; exercises emphasize reasoning and building small programs.

## Sources for language conformance

- [ISO C23 overview](https://en.cppreference.com/w/c/23)
- [GCC C language standards status](https://gcc.gnu.org/projects/c-status.html)
- [Clang C language status](https://clang.llvm.org/c_status.html)

A generated reading PDF is published as [`modern-c23.pdf`](modern-c23.pdf) when the PDF workflow succeeds. The manuscript source remains authoritative; the PDF may lag behind pending workflow runs. Run `sh scripts/build-pdf.sh` locally with Pandoc and XeLaTeX to rebuild.

License: All original prose and examples © 2026; rights reserved pending a separately selected license.
