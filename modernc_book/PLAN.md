# Editorial Roadmap — Modern C23

**Current state (October 2026):** 19 chapters, a preface, six appendices, and **35 standalone examples**. The current Markdown manuscript contains **83,061 words** including code listings, reference entries, and exercises. It is an independent, original example-first textbook, not an authorized K&R third edition.

## Work completed

- Expanded the original core language chapters with examples, explicit preconditions, arithmetic checks, ownership, and resource bounds.
- Added detailed coverage of C23 language changes and the difference between language features and standard-library implementation support.
- Added three classes of streaming/binary parser: a bounded line reader, incremental UTF-8 scalar decoder, and bounded TLV record decoder.
- Developed atomic counter and SPSC discussion, release/acquire publication, and hosted-thread lifecycle tests.
- Added Appendix F, an alphabetical topic finder with chapter-level references, to help readers navigate the growing book without unstable draft page numbers.
- Added an integrated Chapter 12 capstone for a bounded telemetry receiver with explicit framing, pools, queues, and shutdown semantics.
- Added worked examples of generation-tagged fixed pools and byte-at-a-time binary frame decoding, including model-based and transition testing.
- Added a manuscript structure checker to CI; compiler regression testing uses GCC 14 and Clang 18.
- Maintained a PDF build and publisher that includes all chapters and appendices.

## Next work priorities

1. **Technical correctness:** Audit every example embedded in Markdown, even if not compiled as a standalone program; verify portable pointer, integer and threading claims.
2. **Editorial cohesion:** Consolidate repeated explanation of ownership, representability, and strict parsing into clear cross-references.
3. **Complete exercises:** Add worked solutions for combinations of data structures, formatting, error recovery, and portability.
4. **Visual layout:** Inspect representative rendered pages, including long code blocks, table width, mathematical expressions, and topic-finder layout.
5. **Index:** Retain Appendix F as a chapter-level topic finder during drafting; generate verified page references for a final printable edition.
6. **Publication target:** Reach approximately 200–230 high-quality typeset pages through necessary technical explanation, not padding.

## Verification rules

A file or workflow existing in GitHub is not evidence that it passed. Always inspect completed CI runs for the relevant commit. A passing host test does not establish interrupt safety, DMA coherency, target ABI compatibility, or real-time deadlines.

A subsequent technical expansion adds multi-key sorting, `_Generic` dispatch, a fixed-header correction, separate object compilation, a full word-index trace and stronger boundary tests, and additional standard-library API contracts. The source now totals 83,061 words including code, exercises and appendices; the last independently measured PDF before this latest material was 187 pages.

The Markdown tree is authoritative. The tracked `modern-c23.pdf` is an output artifact and can lag when PDF workflows are queued or superseded. Measure the number of pages from the actual completed build rather than inferring it from manuscript word count.
