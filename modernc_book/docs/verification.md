# Verification record

## 26 October 2026 — targeted packet decoder

- Source: `examples/packet_decode.c`
- Environment: GCC 14.2.0, Linux
- Mode: `gcc -std=c2x -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`
- Result: compiled successfully and printed `packet_decode: OK`.
- Coverage: happy path, deliberately corrupted checksum, truncated input, null buffer.
- Limitations: This is a targeted smoke test, not full coverage of every packet shape. GCC's C2x switch selects its developing C23 dialect, but does not prove full ISO C23 library conformance.

The rest of the programs are not yet recorded as compile-tested. Do not extrapolate this verification result to the whole book. GitHub updates were made using the repository connector; clone access from the local build environment was unavailable.


## 26 October 2026 — C23 basics walkthrough

- Source: `examples/c23_basics.c` (matching the complete listing in Chapter 9).
- Environment: GCC 14.2.0 and Clang 17.0.0 on Linux.
- Invocation: `-std=c23 -Wall -Wextra -Wpedantic -Werror`.
- Result: both compilers built and executed the example; output was `c23_basics: OK`.
- Limitation: This covers only the selected C23 syntax in the example. It does not establish `#embed`, `constexpr`, newer library-header support, or the status of the full CI matrix.
