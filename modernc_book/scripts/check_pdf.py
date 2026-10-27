#!/usr/bin/env python3
"""Preflight the generated PDF for truncation and off-page text boxes.

This is a machine-readable layout check, not a visual proof that every page
looks good. It intentionally permits unusual short pages (title, TOC, etc.).
"""
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

pdf = Path(sys.argv[1]) if len(sys.argv) == 2 else Path("build/modern-c23.pdf")
if not pdf.is_file() or pdf.stat().st_size == 0:
    raise SystemExit(f"Missing or empty PDF: {pdf}")

info = subprocess.run(["pdfinfo", str(pdf)], capture_output=True, text=True, check=True).stdout
m = re.search(r"^Pages:\s+(\d+)", info, re.MULTILINE)
if m is None:
    raise SystemExit("pdfinfo did not report a page count")
page_count = int(m.group(1))
if not 180 <= page_count <= 260:
    raise SystemExit(f"Unexpected page count {page_count}; inspect document inputs")

result = subprocess.run(["pdftotext", "-bbox", str(pdf), "-"],
                        capture_output=True, check=True)
xml = ET.fromstring(result.stdout)
pages = [elem for elem in xml.iter() if elem.tag.endswith("page")]
if len(pages) != page_count:
    raise SystemExit(f"Page count mismatch: pdfinfo {page_count}, bbox {len(pages)}")

word_count = 0
empty_pages = []
off_page = []
edge_words = []
for page_number, page in enumerate(pages, 1):
    width = float(page.attrib["width"])
    height = float(page.attrib["height"])
    words = [node for node in page.iter() if node.tag.endswith("word")]
    word_count += len(words)
    if not words:
        empty_pages.append(page_number)
    for word in words:
        x0 = float(word.attrib["xMin"])
        x1 = float(word.attrib["xMax"])
        y0 = float(word.attrib["yMin"])
        y1 = float(word.attrib["yMax"])
        if (x0 < -0.75 or y0 < -0.75 or
            x1 > width + 0.75 or y1 > height + 0.75):
            if len(off_page) < 12:
                off_page.append((page_number, (word.text or "")[:35],
                                 round(x0, 1), round(x1, 1), round(width, 1)))
        elif (x0 < 12 or x1 > width - 12) and len(edge_words) < 12:
            edge_words.append((page_number, (word.text or "")[:35]))

if word_count < 12000:
    raise SystemExit(f"Too little searchable text: {word_count} words")
print(f"PDF preflight: {page_count} pages; {word_count} text tokens")
if empty_pages:
    print(f"REVIEW empty pages: {empty_pages[:20]}")
if edge_words:
    print(f"REVIEW near-edge words: {edge_words}")
if off_page:
    for item in off_page:
        print(f"ERROR off-page text: {item}", file=sys.stderr)
    raise SystemExit("PDF contains extracted text outside page bounds")
print("PDF preflight passed; manual visual review remains required")
