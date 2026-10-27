#!/usr/bin/env python3
"""Small dependency-free structural checks for the Modern C23 manuscript."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
errors = []
chapters = sorted((ROOT / "chapters").glob("*.md"))
appendices = sorted((ROOT / "appendices").glob("*.md"))

numbers = [int(p.name[:2]) for p in chapters if re.match(r"\d\d-", p.name)]
if numbers != list(range(1, 20)):
    errors.append(f"Expected numbered chapters 01..19; found {numbers!r}")

letters = [p.name[:1] for p in appendices]
if letters != list("ABCDEF"):
    errors.append(f"Expected appendices A..F; found {letters!r}")

manuscript = [ROOT / "frontmatter" / "preface.md", *chapters, *appendices]
for path in manuscript:
    if not path.is_file():
        errors.append(f"Missing manuscript file: {path}")
        continue
    text = path.read_text(encoding="utf-8")
    if not text.strip():
        errors.append(f"Empty manuscript file: {path}")
    if "\x00" in text:
        errors.append(f"NUL byte in Markdown file: {path}")
    fences = sum(1 for line in text.splitlines()
                 if re.match(r"^\s*`{3,}", line))
    if fences % 2:
        errors.append(f"Unbalanced backtick code fences in {path}")

    if path in chapters:
        chapter_number = int(path.name[:2])
        found = [int(n) for n in re.findall(
            rf"^## {chapter_number}\.(\d+)\s", text, re.MULTILINE)]
        expected = list(range(1, len(found) + 1))
        if found != expected:
            errors.append(
                f"Nonconsecutive sections in {path.name}: "
                f"{found}, expected {expected}")

# Links are resolved relative to the file that contains them.
# URL fragments are deliberately excluded: this checks the file destination,
# not whether a specific heading anchor exists after rendering.
linked_documents = [
    ROOT / "README.md", ROOT / "examples" / "README.md",
    ROOT / "PLAN.md", ROOT / "docs" / "editorial-status.md",
    *manuscript,
]
link_count = 0
for source in linked_documents:
    if not source.is_file():
        errors.append(f"Missing linked document: {source}")
        continue
    content = source.read_text(encoding="utf-8")
    for raw in re.findall(r"(?<!!)\[[^\]]+\]\(([^)]+)\)", content):
        destination = raw.split("#", 1)[0].strip()
        if not destination or re.match(r"^[a-z][a-z0-9+.-]*:", destination):
            continue
        if destination.startswith("/"):
            errors.append(f"Absolute local path in {source}: {destination}")
            continue
        resolved = (source.parent / destination).resolve()
        if not resolved.is_relative_to(ROOT.resolve()):
            errors.append(f"Local link leaves book: {source}: {destination}")
            continue
        link_count += 1
        if not resolved.exists():
            errors.append(f"Broken local link: {source}: {destination}")
print(f"Checked {link_count} internal Markdown links")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
match = re.search(r"^all:\s+([^\n]+)", makefile, flags=re.M)
if match is None:
    errors.append("Makefile has no all target")
else:
    programs = match.group(1).split()
    tests = ((ROOT / "tests" / "test_examples.sh").read_text(encoding="utf-8")
             + (ROOT / "tests" / "test_word_index.sh").read_text(encoding="utf-8"))
    for name in programs:
        if not re.search(rf"^{re.escape(name)}:", makefile, flags=re.M):
            errors.append(f"Missing Makefile recipe for {name}")
        if f"./{name}" not in tests:
            errors.append(f"Missing regression invocation for {name}")
    if len(set(programs)) != len(programs):
        errors.append("Duplicate executable target in Makefile all")
    print(f"Checked {len(programs)} executable targets")

# Check that the draft topic finder refers only to existing chapters and appendices.
finder = ROOT / "appendices" / "F-topic-finder.md"
if finder.is_file():
    index = finder.read_text(encoding="utf-8")
    entries = [line for line in index.splitlines() if line.startswith("- **")]
    if len(entries) < 75:
        errors.append(f"Topic finder unexpectedly short: {len(entries)} entries")
    for line in entries:
        # Match both singular/plural and ranges; check all number endpoints.
        for hit in re.finditer(r"\\bChapters?\\s+([0-9]+)(?:[–-]([0-9]+))?", line):
            for value in (hit.group(1), hit.group(2)):
                if value is not None and not 1 <= int(value) <= len(chapters):
                    errors.append(f"Invalid topic-finder chapter reference: {line}")
        for letter in re.findall(r"\\bAppendix\\s+([A-Z])\\b", line):
            if letter not in letters:
                errors.append(f"Invalid topic-finder appendix reference: {line}")
    print(f"Checked {len(entries)} topic-finder entries for reference ranges")
else:
    errors.append("Missing Appendix F topic finder")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)
print(f"Manuscript structure OK: {len(chapters)} chapters, "
      f"{len(appendices)} appendices, {len(manuscript)} text files")
