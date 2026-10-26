#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
pandoc --metadata-file=metadata.yaml \
  preface.md \
  chapters/01-why-tinyml.md \
  chapters/02-mathematics.md \
  chapters/03-quantization.md \
  chapters/04-runtime.md \
  chapters/05-memory.md \
  chapters/06-dsp.md \
  chapters/07-realtime.md \
  chapters/08-optimization.md \
  chapters/09-workflow.md \
  chapters/10-end-to-end.md \
  appendices.md \
  --from=markdown+tex_math_dollars+raw_tex \
  --standalone --toc --toc-depth=1 --number-sections --top-level-division=chapter \
  --pdf-engine=xelatex --highlight-style=tango \
  -o TinyML_for_Embedded_Software_Engineers_Draft.pdf
