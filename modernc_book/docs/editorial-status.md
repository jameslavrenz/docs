# Modern C23 — Editorial and Verification Status

## Source inventory

- 19 chapters and a preface
- Six appendices, including worked solutions and an alphabetical topic finder
- 83,061 Markdown words including code and exercises
- 35 standalone C examples in the Makefile and regression script
- Structural checks of chapter and subsection numbers, appendix letters, Markdown fences, internal links throughout the manuscript, and executable-test references

## Latest technical additions

Chapters 5, 7, 10, 12, 13 and 15 were expanded for generation-tagged resource pools, incremental framing, logical ownership, bounds proofs, and model-based/state-transition testing. New `slot_pool.c` and `frame_stream.c` programs are included in the executable regression suite; Appendix C supplies worked solutions. Chapter 12 now offers a complete bounded telemetry receiver design exercise integrating the parser, fixed pool, handle queue and shutdown contracts. The manuscript now contains approximately 83,061 words including code and exercises.


The strict incremental UTF-8 decoder (`examples/utf8_stream.c`) distinguishes incomplete, complete and invalid sequences and preserves the previous scalar on non-success. The bounded TLV decoder (`examples/tlv_decode.c`) validates headers and lengths before reading, rejects records beyond fixed resource limits, and publishes the result only after validating the whole input.

Chapters 18 and 10 contain the detailed explanations, with related worked solutions in Appendix C. Appendix F improves navigation during the draft phase.

Chapter 8's header example was corrected to match the compiled module, including `<stdbool.h>` for its boolean result. The word-index tests now cover an exact 31-character token, a rejected 32-character token at EOF, and reverse alphabetical insertion. Two new test programs verify comparator and `_Generic` behavior.

## Verification

GitHub Actions runs C23 examples using GCC 14 and Clang 18 with warnings as errors (with one targeted Clang 18 workaround for its C23 binary literal diagnostic). The new source examples have passed intermediate runs. Refer to the latest workflow result for confirmation of the final editorial commit.

The extended manuscript checker passed with all internal links and consecutive chapter section numbers. GCC 14 and Clang 18 compiled 13 complete chapter listings and tested all 35 standalone programs; a separate GCC 14 UndefinedBehaviorSanitizer job also passed the 35-program suite. These tests do not establish all semantics of every illustrative fragment or target-hardware behavior.

## PDF status

The PDF workflow at commit `4413f2d` (run 40580124200) completed successfully and published a **206-page** reading edition. Automated checks found no off-page text. The contents now fits on one page; the PDF contains 26 navigable chapter/appendix bookmarks. Short code examples are protected against split page breaks. The PDF remains a technical-review draft pending exhaustive visual and page-numbered-index verification.

## Remaining acceptance criteria

Technical audit of illustrative fragments (13 complete listings compile on two compilers; one optional-library listing remains excluded), visual PDF inspection, verified page-numbered index, and a full compatibility matrix for optional C23 language/library features.

## 27 October 2026 — publication-quality review continuation

- Added `scripts/pdf_review_report.py`, a page-by-page TSV review worksheet based on PDF text bounding boxes. The PDF workflow now generates and uploads this inspection artifact; it flags candidates for human inspection, but does not claim a completed visual review.
- Promoted three self-contained Chapter 19 examples (header include guard, `static_assert` with `<limits.h>`, and `_Generic` floating dispatch) to the dual-compiler strict listing checker using `c compile-check` fences. Context-dependent surrounding fragments are still illustrative, not independently compile-tested.
- Existing Appendix F remains a chapter-level topic finder, **not** a conventional verified page-numbered index. Do not advertise it as such in the reading edition.
- The latest PDF and examples workflows must be checked at their final commit and completion status before publication. No final full-document visual sign-off is claimed here.

## 27 October 2026 — print-layout and index review

- Rendered and inspected representative title, contents, chapter, code, equation, table, and topic-finder pages from the previous 205-page edition. The compact contents formatting now removes the nearly blank second contents page without changing the body font.
- The 206-page edition has a one-page table of contents, 26 PDF bookmarks, and zero off-page words detected by `check_pdf.py` (GitHub Actions run 40580124200).
- A page-transition inspection found two remaining code listings that continue across page boundaries: a 29-line cleanup-path function and the extended word-index program. This is an improvement from eleven cross-page listing transitions in the prior PDF. This is not a claim of full page-by-page visual sign-off.
- Alphabetized all 142 topic-finder entries and added an ordering/uniqueness check to CI. Appendix F continues to use **chapter locators**, not verified per-topic page references.
- Three more self-contained chapter fragments were promoted into strict compile checking (rotate-right, `restrict` array addition, and callback byte sink). GCC 14 / Clang 18 checks now report **46 Markdown listings checked, two skipped, 129 intentionally contextual or not-yet-compiled C blocks**, and all 35 standalone programs tested successfully.
- Remaining publication criteria: finish the contextual-fragment technical audit, review the printed page transitions and other representative layouts, and generate/verify a conventional page-numbered index after pagination is frozen.

## 27 October 2026 — page-index and C23 snippet verification

- Added `scripts/page_index_audit.py` to compare Appendix F's 142 entries with text extracted from the *rendered* PDF, searching only the referenced chapters and appendices. GitHub Actions uploads `page-index-audit.tsv` for inspection. The corrected PDF run (40584580141) found 95 literal-label page matches and 47 entries requiring editorial locators. These are **candidate page references**, not a completed or curated page-numbered index. An exact label need not appear in the relevant discussion, and an occurrence alone does not guarantee the most useful reference page.
- Promoted three standalone structure illustrations (pointer/length slice, flexible-array record, callback byte-sink) to strict dual-compiler listing verification. The compiler suite at commit `9c0f6e4` passed with 49 Markdown listings checked, two optional snippets skipped and 126 contextual blocks remaining.
- Corrected a false impression in the topic finder: Chapter 2 now introduces compound literals explicitly. The example is placed inside a function, where its stated block-scope lifetime applies, and is syntax-checked by the C23 compiler workflow. Its final combined CI result must be verified after the change.
- The authoring source retains chapter locators in Appendix F until a human-vetted subject index with accurate printed pages has replaced them; do not substitute chapter-opening page numbers for subject locations.

## 27 October 2026 — indexed-page verification and math rendering

- Corrected a page-index audit defect: the earlier parser overwrote Chapter 1 and Chapter 2 boundaries when numbered list items began pages 127 and 137. Chapter/appendix boundaries are now matched against the actual manuscript H1 titles, with dedicated regression tests. Verified at PDF run 40584580141, 206 pages, 95 literal-label page matches and 47 topics still needing editorial page selection.
- Inspected PDF page 127 and confirmed a rotate formula was printed as literal LaTeX despite passing the off-page check. Corrected double-dollar display-math fences in Chapters 14 and 18 and inline math in Chapter 14; verified the next published PDF rendered the equations as mathematical notation (run 40585326277, 206 pages).
- Added a source preflight to reject lone dollar-sign display fences and odd counts of doubled-dollar fences outside code blocks.
- Added 40 source-evidence phrases for nonliteral topic labels. The extended audit keeps literal matches distinct from evidence-backed candidate pages, refuses missing support in the cited chapter/appendix, and produces a separate human-readable page-index preview. This is not a finished index; its latest CI result must be checked before using the new candidate totals.
