# Editorial and Verification Status — October 2026

## Manuscript

- 19 chapters, five appendices, and a preface
- Approximately 61,000 source words including code listings and exercises
- 29 standalone examples integrated with `make check`
- Latest additions: checked function contracts, robust line reading, hosted atomic threads, C23 feature probing, and corresponding worked solutions

## Source and tests

The expanded manuscript and complete examples are committed to `main` under `modernc_book/`. The GitHub Actions matrix builds examples under GCC 14 and Clang 18 using an explicit C23 dialect and strict warnings. See the workflow's latest run for the current result.

The two new examples have specific limits. `line_reader.c` uses hosted `FILE *` streams and enforces bounded **storage**, not bounded time spent draining a long record. `thread_counter.c` requires the optional hosted C threads facility and does not establish interrupt safety or an RTOS timing guarantee.

## PDF publication

The PDF workflow includes the preface, 19 chapters and five appendices, then measures the actual resulting PDF. The previous confirmed PDF was 124 pages; newer source changes must not be assumed typeset until a later successful publication is verified.

## Outstanding acceptance criteria

- Technical audit of all illustrative fragments and standard-library portability claims
- Cross-compiler tests for advanced optional C23 features
- Visual inspection of the generated PDF and page-break quality
- Useful book index, complete cross-references, and editorial consistency pass
- Expansion toward the intended roughly 200–230-page reading edition without padding or redundant prose
