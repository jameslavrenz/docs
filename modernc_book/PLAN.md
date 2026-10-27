# Editorial Roadmap — Modern C23

**Current milestone:** comprehensive second-pass revision completed on 26 October 2026. The repository now has 19 chapters, 5 appendices, a preface, and 27 standalone example programs. Chapter, appendix and preface Markdown total approximately 55,800 words, including code and exercises. The 200–230-page target is an editorial objective, not a verified page count.

## Completed in this pass

- Expanded every chapter with deeper treatment of its main contracts, edge cases, portability and tests.
- Expanded the three existing appendices, and added a terminology glossary and C23 migration/conformance checklist.
- Wrote a coherent preface and clarified the independent authorship and audience.
- Corrected chapter 11 and 12 heading hierarchy and section numbering.
- Corrected the standard-library description of C23 `<stdbit.h>`.
- Extended the PDF build to include the preface, all chapters, and all appendices.
- Updated GitHub Actions to avoid attempting to publish a stale PDF if newer source commits exist.

## Remaining required work

1. **Technical verification.** Compile every standalone example on at least two real C23-capable toolchains, capture actual CI outcomes, and repair failures. Add representative sanitizer builds where supported.
2. **Manuscript audit.** Check every complete code listing rather than merely the standalone source files. Verify signatures, includes, numeric assumptions and API contracts.
3. **Exercises.** Develop more complete and independently reviewed solutions, especially for ownership, concurrency, encoding and binary protocols.
4. **Editing.** Normalize overlapping discussion among chapters, correct cross-references, and establish a consistent distinction between complete programs and illustrative fragments.
5. **Book layout.** Generate the current PDF, inspect its actual page count, check fonts and code line breaks, verify the contents hierarchy, and add a usable index.
6. **Final length.** Expand only genuinely thin subjects needed to reach roughly 200–230 typeset pages, without padding or repeating prose.

## Source of truth

The Markdown files are the editable manuscript. `modern-c23.pdf` is a generated reading artifact and may lag while GitHub Actions runs. Source completion and PDF publication are distinct states. A file's presence alone is not proof it was built from the newest commit.

This is an original independent C23 book inspired by a compact example-first pedagogical approach; it is not a third edition of the copyrighted K&R book.

## Subsequent expansion: practical systems examples

Expanded Chapters 8, 11, 13–15, 17 and 19 with additional reasoning, contracts, and engineering examples. Added three complete programs: a checked frame encoder/decoder, a partial-write callback sink, and a multi-file checksum module. Appendix C now explains their correctness conditions and boundary tests. The Makefile and CI script compile and test 24 programs.

PDF page count and publication status must be verified from the newly generated artifact rather than assumed from the manuscript change. The new GitHub Actions PDF build may be waiting behind earlier runs.

## Current expansion: memory, arithmetic, and representation

Detailed additions to Chapters 2, 4, 5, 7, 12 and 14 explain signed decimal boundaries, pointer-to-array types, counted subslices, structure layout and flexible arrays, fixed byte arenas, aligned-object caveats, and rotation operations. Appendix C adds worked derivations and boundary-test tables. Three new programs (`parse_signed.c`, `arena_bytes.c`, `rotate_bits.c`) bring the test suite to 27 standalone programs.

The manuscript and appendices, including code listings, now contain approximately 55,800 words. The most recently measured published PDF before this expansion had 124 pages; verify the regenerated PDF directly rather than estimating pagination from word count.
