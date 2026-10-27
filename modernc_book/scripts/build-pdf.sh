#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
command -v pandoc >/dev/null 2>&1 || { echo 'pandoc required' >&2; exit 1; }
command -v xelatex >/dev/null 2>&1 || { echo 'xelatex required' >&2; exit 1; }
mkdir -p build
pandoc chapters/*.md appendices/*.md --from=gfm --pdf-engine=xelatex \
  --toc --number-sections -V geometry:margin=0.85in -V fontsize=10pt \
  -o build/modern-c23.pdf
echo 'Created build/modern-c23.pdf'
