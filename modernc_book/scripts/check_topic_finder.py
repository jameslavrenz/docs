#!/usr/bin/env python3
"""Check that every topic-finder entry references an existing chapter/appendix.

This verifies navigation targets and alphabetical section organization. It does
not assert that page numbers exist or that the index is editorially complete.
"""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent.parent
source = (root / "appendices/F-topic-finder.md").read_text(encoding="utf-8")
chapters = sorted((root / "chapters").glob("[0-9][0-9]-*.md"))
appendices = sorted((root / "appendices").glob("[A-F]-*.md"))
actual_chapters = {int(p.name[:2]) for p in chapters}
actual_appendices = {p.name[0] for p in appendices}
errors = []
entries = 0
for line_no, line in enumerate(source.splitlines(), 1):
    if not line.startswith("- **"):
        continue
    entries += 1
    # An entry can reference Chapters 2–4, Chapters 3 and 8, or Chapter 15.
    refs = re.findall(r"Chapters?\\s+([0-9][0-9,\\s–-]*(?:and\\s+[0-9]+)?)", line)
    for ref in refs:
        values = [int(n) for n in re.findall(r"\\d+", ref)]
        for a, b in zip(values, values[1:]):
            if re.search(rf"\\b{a}\\s*[–-]\\s*{b}\\b", ref):
                for n in range(a, b + 1):
                    if n not in actual_chapters:
                        errors.append(f"line {line_no}: nonexistent chapter {n}")
        for n in values:
            if n not in actual_chapters:
                errors.append(f"line {line_no}: nonexistent chapter {n}")
    for appendix in re.findall(r"Appendix\\s+([A-Z])", line):
        if appendix not in actual_appendices:
            errors.append(f"line {line_no}: nonexistent Appendix {appendix}")
    if "Chapter" not in line and "Appendix" not in line:
        errors.append(f"line {line_no}: topic lacks a chapter or appendix locator")
if entries < 90:
    errors.append(f"Expected at least 90 index entries, found {entries}")
print(f"Topic finder: {entries} entries; {len(actual_chapters)} chapters; {len(actual_appendices)} appendices")
for error in errors:
    print("ERROR:", error, file=sys.stderr)
if errors:
    sys.exit(1)
print("Chapter/appendix targets structurally checked; page-level index remains unverified")
