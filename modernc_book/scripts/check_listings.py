#!/usr/bin/env python3
"""Compile standalone C listings printed in the book's Markdown chapters.

A block with a main function and its own includes is a complete program.
Self-contained function fragments may be tagged `c compile-check` in Markdown.
Only tagged fragments are independently checked; contextual snippets remain
illustrative. These are syntax checks, not runtime tests or proofs.
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
contextual = []
with tempfile.TemporaryDirectory(prefix="modernc-listings-") as temp:
    scratch = Path(temp)
    for chapter in sorted((ROOT / "chapters").glob("*.md")):
        source = chapter.read_text(encoding="utf-8")
        blocks = list(re.finditer(
            r"^```c(?P<info>[^\n]*)\n(?P<code>.*?)^```\s*$",
            source, flags=re.MULTILINE | re.DOTALL))
        for number, block in enumerate(blocks, 1):
            code = block.group("code")
            is_fragment = "compile-check" in block.group("info").split()
            has_main = re.search(r"\bint\s+main\s*\(", code) is not None
            if not (is_fragment or has_main):
                contextual.append((chapter.name, number, code.strip().splitlines()[0][:90] if code.strip() else '(empty)'))
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
            extra = ["-Wno-unused-function"] if is_fragment else []
            command = COMPILER + FLAGS + extra + ["-fsyntax-only", str(source_path)]
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
print(f"Context-dependent C snippets not independently compiled: {len(contextual)}")
for name, number, preview in contextual:
    print(f"CONTEXT {name} C block {number}: {preview}")
print(f"Markdown listings: {checked} checked; {skipped} skipped; "
      f"{len(failed)} failed")
if checked < 35:
    print("Too few complete chapter listings found", file=sys.stderr)
    sys.exit(1)
if failed:
    sys.exit(1)
