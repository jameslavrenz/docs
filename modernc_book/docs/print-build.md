# PDF reading edition

The Markdown chapters and appendices are the book's source of truth. Generate a reading PDF on a machine with Pandoc and a LaTeX installation providing XeLaTeX:

```sh
sh scripts/build-pdf.sh
```

The output file is `build/modern-c23.pdf`. It is a reading draft, not yet a print-ready edition. Page count must be measured on the generated PDF; the current manuscript does not yet meet the approximately 200–230-page goal. Chapter numbering is currently authored in headings and may be duplicated if automatic numbering is enabled; this will be normalized during final typesetting.

The build has not been verified in this GitHub editing session. Before publishing, inspect actual PDF pages, font rendering, code-block breaks, index, table of contents, and cross-references.
