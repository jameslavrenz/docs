#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

static bool read_u32_le(const unsigned char *bytes, size_t count,
                        uint32_t *out)
{
    if (bytes == nullptr || out == nullptr || count < 4) return false;
    uint32_t value = (uint32_t)bytes[0]
                   | ((uint32_t)bytes[1] << 8)
                   | ((uint32_t)bytes[2] << 16)
                   | ((uint32_t)bytes[3] << 24);
    *out = value;
    return true;
}

int main(void)
{
    const unsigned char bytes[] = {0x78, 0x56, 0x34, 0x12};
    uint32_t value = 0;
    if (!read_u32_le(bytes, sizeof bytes, &value)
        || value != UINT32_C(0x12345678))
        return 1;
    printf("decoded: %08" PRIx32 "\n", value);
    return 0;
}
