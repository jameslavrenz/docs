#include "checksum.h"
#include <stdio.h>

int main(void)
{
    const unsigned char bytes[] = {0x11, 0x22, 0x44};
    unsigned char checksum = 77;
    if (!book_xor_checksum(bytes, sizeof bytes, &checksum) ||
        checksum != 0x77)
        return 1;

    if (book_xor_checksum(nullptr, 1, &checksum) ||
        checksum != 0x77)
        return 1;

    if (!book_xor_checksum(nullptr, 0, &checksum) ||
        checksum != 0)
        return 1;

    puts("modules_demo: OK");
    return 0;
}
