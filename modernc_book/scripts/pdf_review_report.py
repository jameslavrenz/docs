#!/usr/bin/env python3
"""Produce a page-by-page PDF inspection queue; not a substitute for visual review.

Usage: python3 scripts/pdf_review_report.py build/modern-c23.pdf
Requires pdftotext (Poppler). Outputs build/pdf-review-report.tsv by default.
"""
import csv
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

pdf = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/modern-c23.pdf")
destination = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("build/pdf-review-report.tsv")
if not pdf.is_file():
    raise SystemExit(f"PDF not found: {pdf}")
raw = subprocess.run(["pdftotext", "-bbox", str(pdf), "-"],
                     capture_output=True, check=True).stdout
root = ET.fromstring(raw)
pages = [node for node in root.iter() if node.tag.endswith("page")]
if not pages:
    raise SystemExit("No PDF pages detected")
records = []
for number, page in enumerate(pages, 1):
    width = float(page.attrib["width"])
    height = float(page.attrib["height"])
    words = [node for node in page.iter() if node.tag.endswith("word")]
    heights = [float(w.attrib["yMax"]) - float(w.attrib["yMin"]) for w in words]
    edge = [w for w in words if float(w.attrib["xMin"]) < 24
            or float(w.attrib["xMax"]) > width - 24]
    off = [w for w in words if float(w.attrib["xMin"]) < -0.75
           or float(w.attrib["xMax"]) > width + 0.75
           or float(w.attrib["yMin"]) < -0.75
           or float(w.attrib["yMax"]) > height + 0.75]
    # A low word count is a review prompt, not automatically a bad page.
    records.append(dict(page=number, words=len(words),
                        min_word_box_height=round(min(heights), 2) if heights else "",
                        near_margin_words=len(edge), off_page_words=len(off),
                        likely_sparse=int(len(words) < 65),
                        inspect_visual="", notes=""))
destination.parent.mkdir(parents=True, exist_ok=True)
with destination.open("w", newline="", encoding="utf-8") as handle:
    writer = csv.DictWriter(handle, fieldnames=list(records[0]), delimiter=chr(9))
    writer.writeheader()
    writer.writerows(records)
print(f"Wrote {len(records)} per-page inspection rows to {destination}")
print("Manually inspect all pages, including code/listing breaks, TOC, and Appendix F.")
print("Text boxes cannot detect overlap, clipped diagrams or unusable index entries.")
