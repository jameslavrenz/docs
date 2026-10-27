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

## 26 October 2026 — C23 CI compatibility repair

- Failure reproduced locally in `examples/spsc_queue.c` using GCC 14.2 and Clang 17 in C23 mode with `-Wall -Wextra -Wpedantic -Werror`.
- Root cause: `ATOMIC_VAR_INIT` was removed from C23; both compilers treated it as undeclared/undefined in the C23 dialect.
- Fix: replace the two `ATOMIC_VAR_INIT(0)` initializer expressions with scalar `0` initializers for the atomic members.
- Retest: the corrected `spsc_queue.c` compiled and ran successfully under both local compilers, printing `spsc_queue: OK`.
- CI result: the GitHub Actions checks for the new commit were not available through the current status interface at the time of review. A locally passing example is not proof of a completed passing workflow.
