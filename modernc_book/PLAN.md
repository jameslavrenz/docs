# Editorial Roadmap — Modern C23

**Status: expanded technical-review draft (October 2026).**

The source manuscript contains 19 chapters, five appendices, a preface, and approximately 61,000 words including examples and exercises. The standalone program suite now contains 29 examples. This is an **independent original textbook** rather than an authorized third edition of K&R.

## Recent completed work

- Detailed treatments of signed arithmetic, arrays and slices, ownership, fixed-capacity arenas, structure representation, byte encoders, and safe rotations.
- Further expansion of Chapters 3, 6, 9, and 16 with function contracts, bounded line input, C23 feature availability, and concurrency reasoning.
- Two additional programs: `line_reader.c` (overlong-record draining and EOF handling) and `thread_counter.c` (hosted atomic increments and thread lifecycle).
- New worked solutions in Appendix C for both programs and for C23 feature probing.
- Compilation and runtime integration through `make check` for GCC and Clang on GitHub Actions.
- A PDF build workflow that generates the complete reading edition and commits the PDF when it passes.

## Remaining editorial priorities

1. **Finish the technical audit.** Compile every complete code listing, not merely examples in `examples/`; check C23 features against the selected compiler and standard library.
2. **Improve structural flow.** Move exercises closer to their relevant concepts, remove repeated descriptions, and normalize headings and cross-references.
3. **Add useful figures.** Develop typeset diagrams for arrays, objects, ownership, queue wraparound, and the C abstract machine; verify line wrapping in the PDF.
4. **Expand selected solutions.** Prioritize parsing, allocation failures, threads, byte layouts, and project-level integration.
5. **Build the index.** Add terminology entries and concept references useful in a printed technical book.
6. **Verify the reading edition.** Measure its actual page count and inspect rendered pages. The target remains about 200–230 pages; a longer document alone is not a quality criterion.

## Verification discipline

A committed test or workflow is not proof that it passed. Record the outcome of actual workflow runs for the current commit. A passing hosted test does not establish interrupt safety, DMA coherency, or real-time latency on an MCU.

The Markdown files are the source of truth. `modern-c23.pdf` is generated and may lag until the document workflow succeeds; do not infer that the PDF includes changes merely because those changes are pushed.
