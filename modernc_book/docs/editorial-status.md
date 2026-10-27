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

The successfully published PDF (run 40511272634) contains **205 pages**, meeting the original 200–230-page target. Subsequent changes in this review pass affect CI and documentation, not the typeset manuscript. The PDF remains a draft pending a page-by-page layout and technical review.

## Remaining acceptance criteria

Technical audit of illustrative fragments (13 complete listings compile on two compilers; one optional-library listing remains excluded), visual PDF inspection, verified page-numbered index, and a full compatibility matrix for optional C23 language/library features.
