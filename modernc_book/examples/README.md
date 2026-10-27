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

The project currently builds **29 standalone programs**, including a separately compiled demonstration with a public header. GitHub Actions also runs the suite with Clang 18.

The `thread_counter.c` example uses the optional hosted C threads interface `<threads.h>` and must be linked with threading support; its Makefile rule supplies `-pthread` on the Linux CI runners. That rule is not necessarily appropriate for a freestanding MCU cross-compiler.

`line_reader.c` uses `tmpfile` to verify bounded line input, recovery after a rejected overlong record, an exact-capacity line, and a final line without newline. A freestanding target may lack file streams entirely.

Some C23 features are treated as **illustrative fragments** rather than build requirements, including `constexpr`, `_BitInt`, `#embed`, `<stdckdint.h>` and `<stdbit.h>`. Test each feature on the intended compiler and C library before relying on it in deployed software.

The test suite verifies specific programs and inputs; it does not guarantee that every short fragment embedded in the chapters is a complete translation unit.
