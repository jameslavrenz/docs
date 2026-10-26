#!/usr/bin/env bash
# Reproducible 6x9-in print-layout book build.
set -euo pipefail
cd "$(dirname "$0")"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT
tex="$work_dir/TinyML_for_Embedded_Software_Engineers.tex"

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
  chapters/11-verification.md \
  chapters/12-reference.md \
  chapters/13-mcu-lab.md \
  appendices.md \
  --from=markdown+tex_math_dollars+raw_tex \
  --standalone --toc --toc-depth=1 --number-sections \
  --top-level-division=chapter --to=latex --highlight-style=tango \
  -o "$tex"

# Pandoc only accepts toc-depth >= 1. A book chapter is depth 0, and a
# chapter-only table of contents is more readable at 6x9-in trim size.
python3 - "$tex" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
needle=r'\setcounter{tocdepth}{1}'
if needle not in s:
    raise RuntimeError('Pandoc table-of-contents depth marker changed')
p.write_text(s.replace(needle,r'\setcounter{tocdepth}{0}',1))
PY

for _ in 1 2; do
  xelatex -interaction=nonstopmode -halt-on-error \
    -output-directory="$work_dir" "$tex" >/dev/null
done
cp "$work_dir/TinyML_for_Embedded_Software_Engineers.pdf" \
   TinyML_for_Embedded_Software_Engineers.pdf
printf 'Built: %s\n' TinyML_for_Embedded_Software_Engineers.pdf
