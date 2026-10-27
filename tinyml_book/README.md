# TinyML for Embedded Software Engineers

**Expanded technical manuscript — October 26, 2026.** A microcontroller-focused, mathematics-first book by James Lavrenz, prepared at **6 × 9 in** trim and aimed at practicing C/C++ firmware engineers. Chapters are independently editable Markdown with LaTeX equations; the book stays focused on MCU inference, primarily Arm Cortex-M, not Linux/MPU/cloud infrastructure.

## Read the book

- [`TinyML_for_Embedded_Software_Engineers.pdf`](TinyML_for_Embedded_Software_Engineers.pdf) — current complete reader/print-layout manuscript.
- [`chapters/`](chapters/) — 15 numbered chapters plus a manager briefing, covering feasibility, inference mathematics, INT8, runtime design, arenas, DSP, real-time firmware, optimization, model workflow, KWS systems, verification, tested reference code, and STM32-class integration.
- [`MANUSCRIPT.md`](MANUSCRIPT.md) — concatenated editable manuscript, generated from sources (do not edit separately).
- [`examples/int8_reference/`](examples/int8_reference/) — original, complete C++20 examples of dense inference, 1-D convolution and a documented rational fixed-point requantizer.
- [`tests/`](tests/) — executable host regression tests with exact and randomized vectors.
- [`figures/`](figures/) — reproducible architecture diagrams.

## Rebuild PDF

Requires Pandoc, XeLaTeX, DejaVu fonts, and a conventional TeX installation.

```sh
./build.sh
```

## Build and test reference code

Requires CMake, a C++20 compiler, and Ninja (or another CMake generator):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/tinyml_demo
```

Optional host sanitizers, with a compiler/runtime that supports them:

```sh
cmake -S . -B build-san -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

## Scope and evidence

- The reference C++20 code and hand-weighted full classifier were compiled and tested in the authoring environment; it is **educational** and uses an explicitly documented rounding convention, not CMSIS-NN/TFLM bit-exact quantization semantics.
- The PDF cites hardware observations already recorded in public KWSKit and NetKit reports. It does **not** claim new board measurements, product-level real-time guarantees, acoustic accuracy, or safety certification.
- Hardware bring-up, end-to-end deployment, independent technical review, professional copyediting, and printed proof remain necessary before calling this a validated commercial edition.

The content is original explanatory writing and illustrative code; external libraries and reference materials remain governed by their own licenses. The material has not yet been independently peer reviewed.

## Publication extension

The additional manager briefing, two-layer end-to-end deterministic inference example, measured case-study analysis, benchmark template, and reproducibility tests are under `chapters/00-manager.md`, `chapters/14-complete-classifier.md`, `chapters/15-case-studies.md`, `examples/full_classifier/`, and `benchmarks/`. The small classifier has **hand-selected weights**, not trained weights; see `PRINT_CHECKLIST.md` for limitations and steps before commercial publication.
