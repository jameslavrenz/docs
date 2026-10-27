#!/usr/bin/env python3
"""Compile standalone C listings printed in the book's Markdown chapters.

A block with a main function and its own includes is treated as a complete
program. Other blocks are illustrative fragments and not independently built.
This is a syntax/type-check audit, not a runtime test or proof of correctness.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
COMPILER = shlex.split(os.environ.get("CC", "cc"))
FLAGS = ["-std=c23", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2"]
if any("clang-18" in word for word in COMPILER):
    # Clang 18 diagnoses standardized C23 binary literals as a GNU extension.
    FLAGS.append("-Wno-gnu-binary-literal")

# The availability of this *library* header is independently platform-specific.
OPTIONAL_HEADERS = ("<stdckdint.h>", "<stdbit.h>")

failed = []
checked = 0
skipped = 0
with tempfile.TemporaryDirectory(prefix="modernc-listings-") as temp:
    scratch = Path(temp)
    for chapter in sorted((ROOT / "chapters").glob("*.md")):
        source = chapter.read_text(encoding="utf-8")
        blocks = re.findall(r"^```c\s*\n(.*?)^```\s*$",
                            source, flags=re.MULTILINE | re.DOTALL)
        for number, code in enumerate(blocks, 1):
            if not re.search(r"\bint\s+main\s*\(", code):
                continue
            if not re.search(r"^\s*#\s*include\b", code, re.MULTILINE):
                print(f"SKIP {chapter.name} C block {number}: no include directives")
                skipped += 1
                continue
            if any(header in code for header in OPTIONAL_HEADERS):
                print(f"SKIP {chapter.name} C block {number}: optional C23 library")
                skipped += 1
                continue

            source_path = scratch / f"{chapter.stem}-{number}.c"
            source_path.write_text(code + "\n", encoding="utf-8")
            command = COMPILER + FLAGS + ["-fsyntax-only", str(source_path)]
            try:
                result = subprocess.run(
                    command, capture_output=True, text=True, timeout=20,
                    check=False)
            except (OSError, subprocess.TimeoutExpired) as exc:
                failed.append((chapter.name, number, str(exc)))
                continue
            checked += 1
            if result.returncode != 0:
                failed.append((
                    chapter.name, number,
                    (result.stdout + result.stderr).strip()))

for name, number, error in failed:
    print(f"FAIL {name} C block {number}\n{error}", file=sys.stderr)
print(f"Markdown listings: {checked} checked; {skipped} skipped; "
      f"{len(failed)} failed")
if checked < 10:
    print("Too few complete chapter listings found", file=sys.stderr)
    sys.exit(1)
if failed:
    sys.exit(1)
