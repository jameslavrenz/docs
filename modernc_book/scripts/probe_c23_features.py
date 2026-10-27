#!/usr/bin/env python3
"""Report optional C23 feature availability on this exact hosted toolchain.

This is an informational portability matrix, not a mandatory conformance
test. Unsupported optional library/toolchain facilities are reported and do
not fail the main book's portable-example test suite.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

COMPILER = shlex.split(os.environ.get("CC", "cc"))
FLAGS = ["-std=c23", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
if any("clang-18" in item for item in COMPILER):
    FLAGS.append("-Wno-gnu-binary-literal")

PROBES = {
    "C23 constexpr scalar objects": """
        int main(void) {
            constexpr int width = 9;
            static_assert(width > 0);
            return width != 9;
        }
    """,
    "C23 bit-precise _BitInt(9)": """
        int main(void) {
            _BitInt(9) s = -200;
            unsigned _BitInt(9) u = 500;
            return !(s == -200 && u == 500);
        }
    """,
    "C23 checked arithmetic <stdckdint.h>": """
        #include <limits.h>
        #include <stdckdint.h>
        int main(void) {
            int value = 0;
            if (ckd_add(&value, 10, 20) || value != 30)
                return 1;
            return !ckd_add(&value, INT_MAX, 1);
        }
    """,
    "C23 bit utilities <stdbit.h>": """
        #include <stdbit.h>
        int main(void) {
            return stdc_count_ones(7u) != 3;
        }
    """,
    "C23 #embed resource inclusion": """
        static const unsigned char data[] = {
        #embed "resource.bin"
        };
        int main(void) {
            return sizeof(data) != 2 || data[0] != 0x12 ||
                   data[1] != 0x34;
        }
    """,
}

with tempfile.TemporaryDirectory(prefix="c23-probes-") as temp:
    root = Path(temp)
    (root / "resource.bin").write_bytes(bytes([0x12, 0x34]))
    for index, (label, code) in enumerate(PROBES.items(), 1):
        source = root / f"feature_{index}.c"
        executable = root / f"feature_{index}"
        source.write_text(code + "\n", encoding="utf-8")
        try:
            built = subprocess.run(
                COMPILER + FLAGS + [str(source), "-o", str(executable)],
                capture_output=True, text=True, timeout=30, check=False)
            if built.returncode == 0:
                run = subprocess.run(
                    [str(executable)], capture_output=True, text=True,
                    timeout=10, check=False)
                if run.returncode == 0:
                    print(f"SUPPORTED: {label}", flush=True)
                    continue
                reason = f"execution exited {run.returncode}"
            else:
                lines = (built.stderr or built.stdout).splitlines()
                reason = next(
                    (line.strip() for line in lines if "error:" in line),
                    "compile/link failed")
            print(f"NOT VERIFIED: {label} ({reason})", flush=True)
        except (OSError, subprocess.TimeoutExpired) as exc:
            print(f"NOT VERIFIED: {label} ({exc})", flush=True)

print("Feature-probe report completed; see results above for this toolchain.")
