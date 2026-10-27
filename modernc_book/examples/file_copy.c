#include <stdio.h>
#include <stdlib.h>

/* Copies stdin to stdout as a byte stream. */
int main(void) {
    unsigned char block[4096];
    for (;;) {
        size_t n = fread(block, 1, sizeof block, stdin);
        if (n != 0 && fwrite(block, 1, n, stdout) != n) {
            fputs("output error\n", stderr);
            return EXIT_FAILURE;
        }
        if (n < sizeof block) {
            if (ferror(stdin)) {
                fputs("input error\n", stderr);
                return EXIT_FAILURE;
            }
            if (feof(stdin)) break;
        }
    }
    if (fflush(stdout) == EOF) {
        fputs("flush error\n", stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
