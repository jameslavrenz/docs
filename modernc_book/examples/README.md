# Compilable examples

Compile with a C23-capable toolchain:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic examples/parse_unsigned.c -o parse_unsigned
./parse_unsigned
cc -std=c23 -Wall -Wextra -Wpedantic examples/byte_decode.c -o byte_decode
./byte_decode
```

Additional examples are referenced in the parent Makefile. Some textbook listings are fragments, not standalone translation units. When a platform's default compiler is too old, choose an installed newer compiler explicitly.
