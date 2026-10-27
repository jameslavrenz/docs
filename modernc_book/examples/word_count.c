#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

int main(void)
{
    unsigned long words = 0;
    bool inside = false;
    int ch;
    while ((ch = getchar()) != EOF) {
        bool separator = ch == ' ' || ch == '\n' || ch == '\t';
        if (separator) {
            inside = false;
        } else if (!inside) {
            if (words == ULONG_MAX) {
                fputs("word counter overflow\n", stderr);
                return 1;
            }
            inside = true;
            ++words;
        }
    }
    if (ferror(stdin)) return 1;
    printf("%lu\n", words);
    return ferror(stdout) ? 1 : 0;
}
