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

readme = (ROOT / "README.md").read_text(encoding="utf-8")
for raw in re.findall(r"(?<!!)\[[^\]]+\]\(([^)]+)\)", readme):
    destination = raw.split("#", 1)[0].strip()
    if not destination or re.match(r"^[a-z][a-z0-9+.-]*:", destination):
        continue
    if not (ROOT / destination).exists():
        errors.append(f"Broken local README link: {destination}")

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

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)
print(f"Manuscript structure OK: {len(chapters)} chapters, "
      f"{len(appendices)} appendices, {len(manuscript)} text files")
