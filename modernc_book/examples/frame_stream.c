/* One-octet-at-a-time framed binary protocol parser. */
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#if CHAR_BIT != 8
#error "The frame format is defined in eight-bit octets"
#endif

enum { FRAME_MAGIC = 0xa5, FRAME_PAYLOAD_MAX = 8 };
enum feed_result { FEED_MORE, FEED_COMPLETE, FEED_REJECTED };
enum parser_phase { EXPECT_MAGIC, EXPECT_LENGTH, EXPECT_PAYLOAD, EXPECT_CHECK };

struct message {
    size_t length;
    unsigned char bytes[FRAME_PAYLOAD_MAX];
};

struct frame_parser {
    enum parser_phase phase;
    struct message candidate;
    size_t received;
    unsigned char xor_check;
};

static void frame_reset(struct frame_parser *parser)
{
    *parser = (struct frame_parser){};
}

/* Frame = A5 length payload XOR(length, payload bytes).
   On FEED_COMPLETE only, writes *out. The caller owns both objects.
   On a rejection, the parser returns to EXPECT_MAGIC; the rejected
   octet is consumed and is NOT reconsidered as the start of a frame.
   State must be initialized to all-zero before the first feed. */
static enum feed_result frame_feed(struct frame_parser *parser,
                                   unsigned char byte, struct message *out)
{
    if (parser == nullptr || out == nullptr)
        return FEED_REJECTED;

    switch (parser->phase) {
    case EXPECT_MAGIC:
        if (byte != FRAME_MAGIC)
            return FEED_REJECTED;
        frame_reset(parser);
        parser->phase = EXPECT_LENGTH;
        return FEED_MORE;

    case EXPECT_LENGTH:
        if (byte > FRAME_PAYLOAD_MAX) {
            frame_reset(parser);
            return FEED_REJECTED;
        }
        parser->candidate.length = byte;
        parser->xor_check = byte;
        parser->received = 0;
        parser->phase = byte == 0 ? EXPECT_CHECK : EXPECT_PAYLOAD;
        return FEED_MORE;

    case EXPECT_PAYLOAD:
        if (parser->received >= parser->candidate.length ||
            parser->received >= FRAME_PAYLOAD_MAX) {
            frame_reset(parser);
            return FEED_REJECTED;
        }
        parser->candidate.bytes[parser->received++] = byte;
        parser->xor_check ^= byte;
        if (parser->received == parser->candidate.length)
            parser->phase = EXPECT_CHECK;
        return FEED_MORE;

    case EXPECT_CHECK:
        if (byte != parser->xor_check) {
            frame_reset(parser);
            return FEED_REJECTED;
        }
        *out = parser->candidate;
        frame_reset(parser);
        return FEED_COMPLETE;
    }
    frame_reset(parser);
    return FEED_REJECTED;
}

/* On end of the transport message, a partial frame is incomplete. */
static bool frame_at_boundary(const struct frame_parser *parser)
{
    return parser != nullptr && parser->phase == EXPECT_MAGIC;
}

int main(void)
{
    struct frame_parser parser = {};
    struct message result = {.length = 77};
    const unsigned char first[] = {0xa5, 3, 10, 20, 30, 3 ^ 10 ^ 20 ^ 30};
    for (size_t i = 0; i < sizeof first; ++i) {
        enum feed_result r = frame_feed(&parser, first[i], &result);
        if ((i + 1 == sizeof first && r != FEED_COMPLETE) ||
            (i + 1 != sizeof first && r != FEED_MORE))
            return 1;
    }
    if (result.length != 3 || result.bytes[0] != 10 ||
        result.bytes[1] != 20 || result.bytes[2] != 30 ||
        !frame_at_boundary(&parser))
        return 1;

    /* Accept a zero-payload frame (the XOR checksum is then zero). */
    const unsigned char empty[] = {0xa5, 0, 0};
    for (size_t i = 0; i < sizeof empty; ++i) {
        enum feed_result r = frame_feed(&parser, empty[i], &result);
        if ((i + 1 == sizeof empty && r != FEED_COMPLETE) ||
            (i + 1 != sizeof empty && r != FEED_MORE))
            return 1;
    }
    if (result.length != 0)
        return 1;

    result.length = 999;
    /* Bad payload length rejects without modifying result. */
    if (frame_feed(&parser, 0xa5, &result) != FEED_MORE ||
        frame_feed(&parser, 9, &result) != FEED_REJECTED ||
        result.length != 999 || !frame_at_boundary(&parser))
        return 1;

    /* A checksum failure is rejected and then a new frame may begin. */
    const unsigned char corrupt[] = {0xa5, 1, 42, 0};
    for (size_t i = 0; i < sizeof corrupt; ++i) {
        enum feed_result r = frame_feed(&parser, corrupt[i], &result);
        if ((i + 1 == sizeof corrupt && r != FEED_REJECTED) ||
            (i + 1 != sizeof corrupt && r != FEED_MORE))
            return 1;
    }
    if (result.length != 999 || !frame_at_boundary(&parser))
        return 1;

    if (frame_feed(&parser, 0xa5, &result) != FEED_MORE ||
        frame_feed(&parser, 2, &result) != FEED_MORE ||
        frame_feed(&parser, 7, &result) != FEED_MORE ||
        frame_at_boundary(&parser) || result.length != 999)
        return 1;
    /* Aborted/truncated frame: reset explicitly before another message. */
    frame_reset(&parser);
    if (!frame_at_boundary(&parser))
        return 1;

    puts("frame_stream: OK");
    return 0;
}
