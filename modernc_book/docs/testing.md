# Validation policy

The manuscript distinguishes complete compilable listings from illustrative fragments. Example source files under `examples/` are intended to compile as ordinary C23 programs.

Run `sh tests/test_examples.sh` with a compiler supporting `-std=c23`. The test script compiles the programs and checks known outputs and selected failures. It is a smoke-test suite, not exhaustive verification. In particular, runtime-specific behavior, embedded hardware, full C23 header support, and sanitizer checks require additional environments.

The manuscript must not claim that all listings are validated merely because the standalone examples build. A future CI workflow should cover GCC and Clang versions supporting C23 as well as a freestanding target for the embedded examples.
