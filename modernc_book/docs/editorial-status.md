# Editorial and Verification Status — 26 October 2026

## Manuscript

- 19 chapters, all expanded in a full second editorial pass.
- 5 appendices, including selected exercise solutions, a glossary and a migration checklist.
- A preface explaining the audience, contracts, portability and source conventions.
- Approximately 34,300 Markdown words in the front matter, chapters and appendices, counting code listings.
- 19 standalone C examples in `examples/`.

## Verified versus unverified

**Verified:** The updated Markdown files, scripts and workflow configuration were committed through the GitHub connector and checked as readable from the repository.

**Not independently verified in this pass:** A completed GitHub Actions test run, successful execution of the full example suite, layout of the newly revised PDF, and whether the regenerated PDF has been committed. An older PDF is present in `modernc_book/modern-c23.pdf`; it must not be assumed to contain the newest revisions until its build is verified.

## C23 specificity

The language specification is ISO/IEC 9899:2024. Features such as `nullptr`, `auto`, `typeof`, `constexpr`, `_BitInt`, `#embed`, `<stdbit.h>` and `<stdckdint.h>` have implementation-specific availability. Test language and library support separately on the intended target.

## Acceptance criteria for a reading edition

A complete reading draft must build from the latest source, include the preface and all five appendices, show a navigable table of contents, have no truncated code listings, and report a measured page count. Publication-ready status additionally requires source and example audits, cross-compiler results, comprehensive proofreading, and an index.
