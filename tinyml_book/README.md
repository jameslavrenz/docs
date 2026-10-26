# TinyML for Embedded Software Engineers

**Draft manuscript (October 26, 2026).** This is the book project for an approximately 100-page technical paperback on **MCU-based neural-network inference**, primarily Arm Cortex-M. It avoids application-processor/cloud implementation topics. Chapters are individual editable Markdown files with LaTeX math; the compiled PDF is intended for a 6 × 9 in. book trim size.

## Manuscript

- `TinyML_for_Embedded_Software_Engineers_Draft.pdf` — combined reader/print-draft PDF.
- `chapters/01-why-tinyml.md` through `chapters/10-end-to-end.md` — ten drafted chapters.
- `preface.md`, `appendices.md` — front and back matter.
- `metadata.yaml` — title, page geometry, LaTeX typography.
- `build.sh` — reproducible Pandoc/XeLaTeX build.

## Build

Requires Pandoc, XeLaTeX, and the DejaVu font family.

```bash
cd tinyml_book
./build.sh
```

## Editorial status

This is a **complete first-pass technical draft**, not a professionally edited final edition. A publishable edition still needs independent technical review, all-code compilation/tests, technical diagrams, consistent model-dependent numerical test vectors, a copyedit, printing proof, and updated citations. The chapter's small inference-kernel code is explanatory and not claimed as a fully implemented production runtime. Hardware timing and memory observations are attributed to the specific existing KWSKit/NetKit repository sessions and should not be interpreted as general MCU capability or worst-case guarantees.

The scope is narrow by design: neural-network *inference* on resource-constrained microcontrollers. Training is addressed only insofar as it affects deployed artifacts. The book does not attempt to cover cloud AI, GPU training stacks, or Cortex-A Linux inference.
