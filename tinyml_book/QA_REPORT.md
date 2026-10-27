# TinyML book — technical and production QA record

**Revision:** Expanded manuscript, October 26, 2026

## Verified in the preparation environment

- Rebuilt the manuscript to a searchable **112-page**, **6 × 9-inch** PDF with Pandoc and XeLaTeX; mathematical expressions are LaTeX typeset.
- Confirmed all 15 numbered chapters, one manager briefing, eight lettered appendices, and a concise table of contents are present.
- Compiled C++20 `dense`, `conv1d`, and `requantize` examples with host GNU C++ (`-Wall -Wextra -Wpedantic -Werror`).
- Executed deterministic numeric tests and **100,000 seeded rational-requantization cases** with a high-precision host oracle.
- Repeated host tests with AddressSanitizer + UndefinedBehaviorSanitizer enabled.
- Confirmed demo output `[5, -2, 4]` against the hand-derived vectors.
- Added and executed two-layer fixed-weight classifier demonstration and regression tests in Debug, Release and sanitizer-enabled host builds.
- Added manager briefing, KWSKit/NetKit case-study analysis, four new Graphviz diagrams, benchmark CSV template, licensing notice and print checklist.
- Cross-checked the existing KWSKit Zephyr inference observations cited in chapters 1/10 against its public hardware validation notes (54.994-ms average, 55.054-ms observed maximum, particular normal run).
- Cross-checked the NetKit STM32 MNIST-CNN accelerated and reference results in chapter 8 against its repository's peer comparison documentation (95.3 ms / 95.5 ms accelerated; 336.2 ms / 2593.5 ms reference).

## Not yet verified; do not imply otherwise

- No Arm MCU cross-compiler or physical board was available in the authoring environment for an on-target execution or benchmark of the new reference code.
- The shipped reference operators are not a TFLM/CMSIS-NN bit-exact implementation. The new two-layer classifier performs complete fixed-weight inference but is not trained or statistically validated.
- No new keyword spotting accuracy, acoustic validation, target latency, energy, SRAM high-water, or field reliability measurements were performed.
- The book has not undergone an independent engineering peer review, professional copyedit, or physical print proof.
- Printed-book compliance with any specific vendor's interior, cover, bleed, or ISBN requirements has not been validated.

## Publication assessment

The 112-page manuscript is technically substantial and self-contained as a reading draft; both companion kernel and classifier code are independently compilable and testable on a host. Before calling it an independently reviewed *production handbook*, validate selected formulas and all code against a chosen production runtime, build and test on the intended STM32 board, obtain independent technical review, and inspect a physical printing proof.

## Reproduction

```bash
./build.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

See [README.md](README.md) for sanitizer configuration and important scope limitations.

## Subsequent publication revision

- Added a manager-focused briefing, 4 new reproducible figures, two-layer fixed-weight classifier, and a measured-project interpretation chapter.
- Both host CTest suites pass in the local release configuration. No **trained** full-model inference or physical-board integration is claimed by the new classifier.
- Independent technical peer review, professional copyedit, printed proof, cover artwork, commercial rights decisions, new real-hardware measurements, and model-training-to-target reproducibility remain outstanding.
