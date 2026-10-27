# PDF reading edition and print verification

The Markdown manuscript is authoritative. Generate the PDF with:

```sh
sh scripts/build-pdf.sh
python3 scripts/check_pdf.py build/modern-c23.pdf
```

The last confirmed published reading edition reported 205 pages (GitHub Actions run 40511272634). Page counts must be remeasured after changes: they are properties of the produced PDF, not of its Markdown word count.

## Print acceptance checklist

1. Check the completed PDF workflow for the exact source commit; do not equate a queued build with a passed build.
2. Use `pdfinfo` to record total pages, media dimensions and embedded fonts (`pdffonts`).
3. Run `scripts/check_pdf.py` for missing, empty and off-page text. Its successful completion is necessary but does **not** establish professional typography.
4. Visually inspect front matter, contents, the beginning and end of every chapter, several pages with wide code, tables, equations, long identifiers, and all pages of Appendix F. Look for orphaned headings, tiny type, cropped code, broken tables and excessive whitespace.
5. Confirm that the contents and all appendix references match the actual printed headings and that Appendix F points to existing chapters.
6. Distinguish the current **chapter-level topic finder** from a genuine page-numbered index. Appendix F is not yet a verified page-numbered index. Page-numbered entries must be regenerated and checked against the final, frozen PDF pagination.
7. Regenerate the PDF after any typesetting or index edits and repeat all checks.

A text-extraction pass alone cannot establish whether a word is clipped, a table is legible, or a code block is split poorly. For those claims, render and examine the actual pages. Keep `modern-c23.pdf` labeled as a review draft until those checks have been completed.
