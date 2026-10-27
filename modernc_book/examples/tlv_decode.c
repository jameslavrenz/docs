/* Bounded TLV parser: explicit lengths, local candidate, one commit. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { TLV_MAX_FIELDS = 4, TLV_MAX_VALUE = 8 };

struct tlv_field {
    unsigned char tag;
    size_t length;
    unsigned char value[TLV_MAX_VALUE];
};
struct tlv_message {
    size_t count;
    struct tlv_field fields[TLV_MAX_FIELDS];
};

/* Grammar: repeated (tag:one byte, length:one byte, length data bytes).
   Tags may repeat and zero-length fields are allowed.
   All failures preserve the caller's message. */
static bool decode_tlv(const unsigned char *bytes, size_t size,
                       struct tlv_message *out)
{
    if (out == nullptr || (size != 0 && bytes == nullptr))
        return false;

    struct tlv_message candidate = {};
    size_t cursor = 0;
    while (cursor < size) {
        if (size - cursor < 2 || candidate.count == TLV_MAX_FIELDS)
            return false;

        unsigned char tag = bytes[cursor];
        size_t length = bytes[cursor + 1];
        cursor += 2;
        if (length > TLV_MAX_VALUE || length > size - cursor)
            return false;

        struct tlv_field *field = &candidate.fields[candidate.count];
        field->tag = tag;
        field->length = length;
        if (length != 0)
            memcpy(field->value, bytes + cursor, length);
        cursor += length;
        ++candidate.count;
    }

    *out = candidate;
    return true;
}

int main(void)
{
    const unsigned char valid[] = {1, 3, 10, 11, 12, 2, 0};
    const unsigned char truncated[] = {1, 4, 10, 11, 12};
    const unsigned char odd[] = {1};
    const unsigned char too_big[] = {1, 9, 0};
    const unsigned char too_many[] = {1,0, 2,0, 3,0, 4,0, 5,0};

    struct tlv_message out = {.count = 99};
    if (!decode_tlv(valid, sizeof valid, &out) || out.count != 2 ||
        out.fields[0].tag != 1 || out.fields[0].length != 3 ||
        out.fields[0].value[0] != 10 || out.fields[0].value[2] != 12 ||
        out.fields[1].tag != 2 || out.fields[1].length != 0)
        return 1;

    const struct {const unsigned char *bytes; size_t length;} bad[] = {
        {truncated, sizeof truncated},
        {odd, sizeof odd},
        {too_big, sizeof too_big},
        {too_many, sizeof too_many}
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        if (decode_tlv(bad[i].bytes, bad[i].length, &out) ||
            out.count != 2 || out.fields[0].tag != 1)
            return 1;
    }
    if (!decode_tlv(nullptr, 0, &out) || out.count != 0)
        return 1;
    if (decode_tlv(nullptr, 1, &out) || out.count != 0)
        return 1;

    puts("tlv_decode: OK");
    return 0;
}
