# Modern C23 — Editorial and Verification Status

## Source inventory

- 19 chapters and a preface
- Six appendices, including worked solutions and an alphabetical topic finder
- 74,139 Markdown words including code and exercises
- 33 standalone C examples in the Makefile and regression script
- Structural checks of chapter numbers, appendix letters, Markdown fences, README links, and executable-test references

## Latest technical additions

Chapters 5, 7, 10, 13 and 15 were expanded for generation-tagged resource pools, incremental framing, logical ownership, bounds proofs, and model-based/state-transition testing. New `slot_pool.c` and `frame_stream.c` programs are included in the executable regression suite; Appendix C supplies worked solutions. The manuscript now contains approximately 74,139 words including code and exercises.


The strict incremental UTF-8 decoder (`examples/utf8_stream.c`) distinguishes incomplete, complete and invalid sequences and preserves the previous scalar on non-success. The bounded TLV decoder (`examples/tlv_decode.c`) validates headers and lengths before reading, rejects records beyond fixed resource limits, and publishes the result only after validating the whole input.

Chapters 18 and 10 contain the detailed explanations, with related worked solutions in Appendix C. Appendix F improves navigation during the draft phase.

## Verification

GitHub Actions runs C23 examples using GCC 14 and Clang 18 with warnings as errors (with one targeted Clang 18 workaround for its C23 binary literal diagnostic). The new source examples have passed intermediate runs. Refer to the latest workflow result for confirmation of the final editorial commit.

The new manuscript checker passed its initial CI test after integration. It detects structural issues, not semantic errors in all prose fragments.

## PDF status

A confirmed PDF published earlier in this session contains **168 pages**. The newest manuscript changes may not yet be reflected in the tracked PDF while its workflow is building. The exact new page count must come from the next completed PDF job.

## Remaining acceptance criteria

Complete code-listing audit; visual PDF inspection; cross-reference cleanup; final index with page numbers; full compatibility matrix for optional C23 language/library features; and enough substantive development to reach the approximately 200–230-page target.
