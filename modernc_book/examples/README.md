# Compilable examples

Compile with a C23-capable toolchain:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic examples/parse_unsigned.c -o parse_unsigned
./parse_unsigned
cc -std=c23 -Wall -Wextra -Wpedantic examples/byte_decode.c -o byte_decode
./byte_decode
```

Additional examples are referenced in the parent Makefile. Some textbook listings are fragments, not standalone translation units. When a platform's default compiler is too old, choose an installed newer compiler explicitly.

## All examples and build verification

Run the complete suite from `modernc_book/`:

```sh
make clean
make CC=gcc-14 CFLAGS='-std=c23 -Wall -Wextra -Wpedantic -Werror -O2' check
```

The project currently builds **35 standalone programs**, including a separately compiled demonstration with a public header. GitHub Actions also runs the suite with Clang 18.

The `thread_counter.c` example uses the optional hosted C threads interface `<threads.h>` and must be linked with threading support; its Makefile rule supplies `-pthread` on the Linux CI runners. That rule is not necessarily appropriate for a freestanding MCU cross-compiler.

`line_reader.c` uses `tmpfile` to verify bounded line input, recovery after a rejected overlong record, an exact-capacity line, and a final line without newline. A freestanding target may lack file streams entirely.

Some C23 features are treated as **illustrative fragments** rather than build requirements, including `constexpr`, `_BitInt`, `#embed`, `<stdckdint.h>` and `<stdbit.h>`. Test each feature on the intended compiler and C library before relying on it in deployed software.

The test suite verifies specific programs and inputs; it does not guarantee that every short fragment embedded in the chapters is a complete translation unit.

The newest complete programs are `utf8_stream.c` (incremental strict UTF-8 scalar decoding, preserving output while incomplete) and `tlv_decode.c` (bounded type-length-value records with a strong failure guarantee). Both are part of the Makefile regression suite.

The manuscript's structural checks run through `python3 scripts/check_manuscript.py`. They verify expected chapters and appendices, Markdown code-fence pairing, local README links, executable targets, and regression-test references. These structural checks do not replace compiling the examples or technically proofreading illustrative fragments.

The additional `slot_pool.c` program demonstrates stale-handle rejection and generation retirement in a fixed pool without heap allocation. `frame_stream.c` demonstrates incremental frame validation, zero-length payloads, checksum rejection, and incomplete input. Both are tested on GCC 14 and Clang 18 by the CI matrix.

The newest complete examples are `sort_records.c` (priority/name/arrival ordering with a comparator that never subtracts potentially overflowing integer values) and `generic_select.c` (type-selected `int`/`long` operations with exactly one evaluation of runtime arguments). The compiler matrix executes both programs under C23.

The CI also checks internal book links and chapter numbering, compiles 13 complete chapter listings separately on GCC 14 and Clang 18, and runs all 35 executable programs under GCC 14 UndefinedBehaviorSanitizer. The optional-library chapter probe is intentionally excluded from the generic listing compilation check.
