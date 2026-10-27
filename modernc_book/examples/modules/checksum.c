#include "checksum.h"

bool book_xor_checksum(const unsigned char *bytes, size_t count,
                       unsigned char *out)
{
    if (out == nullptr || (count != 0 && bytes == nullptr))
        return false;
    unsigned char result = 0;
    for (size_t i = 0; i < count; ++i)
        result ^= bytes[i];
    *out = result;
    return true;
}
