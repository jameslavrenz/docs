# Editorial and Verification Status — 26 October 2026

## Manuscript

- 19 chapters, all expanded in a full second editorial pass.
- 5 appendices, including selected exercise solutions, a glossary and a migration checklist.
- A preface explaining the audience, contracts, portability and source conventions.
- Approximately 55,800 Markdown words in the front matter, chapters and appendices, counting code listings.
- 27 standalone C examples in `examples/`.

## Verified versus unverified

**Verified:** The updated Markdown files, scripts and workflow configuration were committed through the GitHub connector and checked as readable from the repository.

**Verified for the expanded manuscript:** GitHub Actions example tests completed successfully with GCC 14 and Clang 18 on the updated 27-program suite. A passing suite does not mean that all illustrative fragments embedded in the Markdown have been separately compiled.

**Still to verify:** The PDF regeneration containing the latest chapter additions, its actual page count, a visual layout review, and the final index. The older published PDF must not be assumed to contain the latest revisions until its new build succeeds.

## C23 specificity

The language specification is ISO/IEC 9899:2024. Features such as `nullptr`, `auto`, `typeof`, `constexpr`, `_BitInt`, `#embed`, `<stdbit.h>` and `<stdckdint.h>` have implementation-specific availability. Test language and library support separately on the intended target.

## Acceptance criteria for a reading edition

A complete reading draft must build from the latest source, include the preface and all five appendices, show a navigable table of contents, have no truncated code listings, and report a measured page count. Publication-ready status additionally requires source and example audits, cross-compiler results, comprehensive proofreading, and an index.

## Subsequent expansion: practical systems examples

Expanded Chapters 8, 11, 13–15, 17 and 19 with additional reasoning, contracts, and engineering examples. Added three complete programs: a checked frame encoder/decoder, a partial-write callback sink, and a multi-file checksum module. Appendix C now explains their correctness conditions and boundary tests. The Makefile and CI script compile and test 24 programs.

PDF page count and publication status must be verified from the newly generated artifact rather than assumed from the manuscript change. The new GitHub Actions PDF build may be waiting behind earlier runs.

## Current deepening pass

Expanded Chapters 2, 4, 5, 7, 12 and 14 with diagrammatic memory explanations, mathematical bounds checks, allocation ownership, a signed-decimal parser, byte-arena behavior, bit rotation, and detailed exercises. Appendix C adds worked solutions through C.25. The example suite now covers 27 complete programs.
