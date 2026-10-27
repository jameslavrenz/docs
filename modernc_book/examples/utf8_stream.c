/* Incremental UTF-8 scalar decoder: a byte at a time, bounded state. */
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#if CHAR_BIT != 8
#error "This example requires eight-bit octets"
#endif

enum utf8_result { UTF8_MORE, UTF8_COMPLETE, UTF8_INVALID };
struct utf8_decoder {
    uint32_t value;
    uint32_t minimum;
    unsigned remaining;
};

static void utf8_reset(struct utf8_decoder *decoder)
{
    *decoder = (struct utf8_decoder){};
}

/* A successful scalar updates *out; all other results preserve *out.
   Invalid input resets decoder state so the next byte may start anew. */
static enum utf8_result utf8_feed(struct utf8_decoder *decoder,
                                  unsigned char byte, uint32_t *out)
{
    if (decoder == nullptr || out == nullptr)
        return UTF8_INVALID;

    if (decoder->remaining == 0) {
        if (byte < 0x80) {
            *out = byte;
            return UTF8_COMPLETE;
        }
        if (byte >= 0xc2 && byte <= 0xdf) {
            decoder->value = (uint32_t)(byte & 0x1f);
            decoder->minimum = 0x80;
            decoder->remaining = 1;
        } else if (byte >= 0xe0 && byte <= 0xef) {
            decoder->value = (uint32_t)(byte & 0x0f);
            decoder->minimum = 0x800;
            decoder->remaining = 2;
        } else if (byte >= 0xf0 && byte <= 0xf4) {
            decoder->value = (uint32_t)(byte & 0x07);
            decoder->minimum = 0x10000;
            decoder->remaining = 3;
        } else {
            utf8_reset(decoder);
            return UTF8_INVALID;
        }
        return UTF8_MORE;
    }

    if ((byte & 0xc0) != 0x80) {
        utf8_reset(decoder);
        return UTF8_INVALID;
    }

    decoder->value = (decoder->value << 6) | (uint32_t)(byte & 0x3f);
    --decoder->remaining;
    if (decoder->remaining != 0)
        return UTF8_MORE;

    uint32_t scalar = decoder->value;
    bool valid = scalar >= decoder->minimum &&
                 scalar <= UINT32_C(0x10ffff) &&
                 !(scalar >= 0xd800 && scalar <= 0xdfff);
    utf8_reset(decoder);
    if (!valid)
        return UTF8_INVALID;
    *out = scalar;
    return UTF8_COMPLETE;
}

/* A clean stream end is valid only between complete scalar values. */
static bool utf8_finish(const struct utf8_decoder *decoder)
{
    return decoder != nullptr && decoder->remaining == 0;
}

static bool expect_sequence(const unsigned char *bytes, size_t length,
                            uint32_t expected)
{
    struct utf8_decoder d = {};
    uint32_t out = UINT32_C(0xdeadbeef);
    for (size_t i = 0; i < length; ++i) {
        enum utf8_result status = utf8_feed(&d, bytes[i], &out);
        if (i + 1 == length) {
            if (status != UTF8_COMPLETE || out != expected)
                return false;
        } else if (status != UTF8_MORE ||
                   out != UINT32_C(0xdeadbeef))
            return false;
    }
    return utf8_finish(&d);
}

int main(void)
{
    const unsigned char euro[] = {0xe2, 0x82, 0xac}; /* U+20AC */
    const unsigned char smile[] = {0xf0, 0x9f, 0x99, 0x82}; /* U+1F642 */
    const unsigned char overlong[] = {0xe0, 0x80, 0x80};
    const unsigned char surrogate[] = {0xed, 0xa0, 0x80};
    const unsigned char too_large[] = {0xf4, 0x90, 0x80, 0x80};

    if (!expect_sequence((const unsigned char *)"A", 1, 0x41) ||
        !expect_sequence(euro, sizeof euro, 0x20ac) ||
        !expect_sequence(smile, sizeof smile, 0x1f642))
        return 1;

    struct utf8_decoder d = {};
    uint32_t out = 777;
    if (utf8_feed(&d, euro[0], &out) != UTF8_MORE ||
        utf8_finish(&d) || out != 777)
        return 1;
    if (utf8_feed(&d, euro[1], &out) != UTF8_MORE ||
        utf8_feed(&d, euro[2], &out) != UTF8_COMPLETE ||
        out != 0x20ac || !utf8_finish(&d))
        return 1;

    const struct { const unsigned char *bytes; size_t length; } invalid[] = {
        {overlong, sizeof overlong},
        {surrogate, sizeof surrogate},
        {too_large, sizeof too_large}
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        utf8_reset(&d);
        out = 777;
        bool rejected = false;
        for (size_t j = 0; j < invalid[i].length; ++j)
            if (utf8_feed(&d, invalid[i].bytes[j], &out) == UTF8_INVALID) {
                rejected = true;
                break;
            }
        if (!rejected || out != 777 || !utf8_finish(&d))
            return 1;
    }

    /* A malformed continuation resets the decoder. */
    utf8_reset(&d);
    out = 777;
    if (utf8_feed(&d, 0xe2, &out) != UTF8_MORE ||
        utf8_feed(&d, 'X', &out) != UTF8_INVALID ||
        out != 777 || !utf8_finish(&d) ||
        utf8_feed(&d, 'Y', &out) != UTF8_COMPLETE || out != 'Y')
        return 1;
    puts("utf8_stream: OK");
    return 0;
}
