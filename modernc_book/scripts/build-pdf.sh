#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

command -v pandoc >/dev/null 2>&1 || { echo 'pandoc is required' >&2; exit 1; }
command -v xelatex >/dev/null 2>&1 || { echo 'xelatex is required' >&2; exit 1; }
mkdir -p build

# Keep chapter numbers authored in the Markdown; do not apply automatic numbering.
# The source files are ordered lexically by their two-digit chapter prefix.
pandoc frontmatter/preface.md chapters/*.md appendices/*.md \
  --from=gfm --pdf-engine=xelatex --listings --toc --toc-depth=2 \
  --metadata="title:Modern C: A Concise Introduction to C23" \
  --metadata="subtitle:Independent technical-review draft" \
  --metadata="date:October 2026" \
  -V geometry:margin=0.85in -V fontsize=10pt \
  -V listings=true \
  -H scripts/listings-setup.tex \
  -V mainfont='DejaVu Serif' -V monofont='DejaVu Sans Mono' \
  -o build/modern-c23.pdf

test -s build/modern-c23.pdf
# Print the measured reading-edition length when Poppler is installed.
if command -v pdfinfo >/dev/null 2>&1; then
  pdfinfo build/modern-c23.pdf | grep '^Pages:'
fi
echo 'Created build/modern-c23.pdf'
