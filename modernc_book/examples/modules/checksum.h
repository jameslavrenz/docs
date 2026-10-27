#ifndef BOOK_CHECKSUM_H
#define BOOK_CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>

/* XOR of all bytes; output is unchanged when an argument is invalid.
   This simple error-detection example is not cryptographic. */
bool book_xor_checksum(const unsigned char *bytes, size_t count,
                       unsigned char *out);

#endif
