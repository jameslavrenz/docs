#!/usr/bin/env python3
"""Find verified topical page references in the typeset PDF.

A page is reported only if the topic label occurs in a cited chapter/appendix.
This audit is intentionally stricter than a finished subject index: synonyms
that need editorial selection remain unresolved.
"""
import csv
import re
import subprocess
import sys
from pathlib import Path


def pages_in(pdf):
    result = subprocess.run(
        ["pdftotext", "-raw", "-enc", "UTF-8", str(pdf), "-"],
        capture_output=True, text=True, check=True,
    )
    return [page for page in result.stdout.split("\f") if page.strip()]


def normal(text):
    text = re.sub(r"(?<=\w)-\s*\n\s*(?=\w)", "", text)
    return re.sub(r"[^a-z0-9]+", " ", text.casefold()).strip()


def section_bounds(pages, finder):
    """Find only the actual manuscript H1 titles, not numbered list items.

    Reading a printed page's first line as a chapter number is insufficient:
    an ordinary numbered list can begin a new page (for example "2. Types:").
    Compare with the authored H1 titles and reject ambiguous/missing headings.
    """
    book = finder.parent.parent
    chapter_files = sorted((book / "chapters").glob("[0-9][0-9]-*.md"))
    appendix_files = sorted((book / "appendices").glob("[A-F]-*.md"))
    if len(chapter_files) != 19 or len(appendix_files) != 6:
        raise ValueError("Unexpected manuscript chapter or appendix count")

    expected = {}
    for file in chapter_files + appendix_files:
        heading = file.read_text(encoding="utf-8").splitlines()[0]
        if not heading.startswith("# "):
            raise ValueError(f"Missing manuscript H1 in {file}")
        key = ("Ch" + str(int(file.name[:2])) if file in chapter_files
               else "App" + file.name[0])
        expected[key] = normal(heading[2:])

    starts = {}
    for number, page in enumerate(pages, 1):
        first = normal(page.strip().splitlines()[0])
        for key, title in expected.items():
            if first == title:
                if key in starts:
                    raise ValueError(f"Duplicate chapter/appendix title {key} on page {number}")
                starts[key] = number
    if set(starts) != set(expected):
        missing = sorted(set(expected) - set(starts))
        raise ValueError(f"Missing PDF chapter/appendix titles: {missing}")
    keys = ["Ch" + str(n) for n in range(1, 20)] + [
        "App" + c for c in "ABCDEF"
    ]
    positions = [starts[k] for k in keys]
    if positions != sorted(positions) or len(set(positions)) != len(positions):
        raise ValueError(f"Out-of-order PDF chapter/appendix titles: {starts}")
    return {
        key: (starts[key], positions[i+1]-1 if i+1 < len(keys) else len(pages))
        for i, key in enumerate(keys)
    }

def chapter_refs(description):
    refs = []
    for match in re.finditer(r"\bChapters?\s+([\d,\s–-]+(?:and\s+\d+)?)", description):
        phrase = re.sub(
            r"(\d+)\s*[–-]\s*(\d+)",
            lambda m: ",".join(map(str, range(int(m[1]), int(m[2])+1))),
            match[1],
        )
        refs.extend("Ch"+n for n in re.findall(r"\d+", phrase))
    refs.extend("App"+v for v in re.findall(r"\bAppendix\s+([A-F])\b", description))
    return list(dict.fromkeys(refs))


def main(pdf, finder, output):
    pages = pages_in(pdf)
    bounds = section_bounds(pages, finder)
    print("Verified title page map: " + ", ".join(
        f"{name}={first}" for name, (first, _) in bounds.items()))
    normalized_pages = [normal(page) for page in pages]
    rows = []
    for line in finder.read_text(encoding="utf-8").splitlines():
        match = re.match(r"^- \*\*(.+?)\*\*\s*(.*)$", line)
        if not match:
            continue
        label = match[1].rstrip(":").replace(chr(96), "")
        targets = chapter_refs(match[2])
        if not targets or any(t not in bounds for t in targets):
            raise ValueError(f"Unknown chapter/appendix locator for {label}: {targets}")
        needle = normal(label)
        occurrences = []
        for target in targets:
            first, last = bounds[target]
            for page in range(first, last+1):
                if re.search(r"(?<![a-z0-9])"+re.escape(needle)+r"(?![a-z0-9])",
                             normalized_pages[page-1]):
                    occurrences.append(page)
        rows.append({
            "topic": label,
            "cited_sections": ", ".join(targets),
            "verified_pages": ", ".join(map(str, sorted(set(occurrences)))),
            "status": "literal occurrence" if occurrences else "editorial locator needed",
        })
    if len(rows) < 130:
        raise ValueError(f"Expected 142 topic entries, found {len(rows)}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]), delimiter=chr(9))
        writer.writeheader()
        writer.writerows(rows)
    matched = sum(bool(row["verified_pages"]) for row in rows)
    print(f"Page index audit: {len(rows)} topics; {matched} literal matches; "
          f"{len(rows)-matched} need editorial locators; {len(pages)} PDF pages")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("Usage: page_index_audit.py rendered.pdf topic-finder.md report.tsv")
    main(*(Path(value) for value in sys.argv[1:]))
