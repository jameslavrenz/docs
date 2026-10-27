#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#if CHAR_BIT != 8
#error "This protocol is defined in 8-bit octets"
#endif

enum { MAX_PAYLOAD = 64, FRAME_OVERHEAD = 5 };

struct packet {
    size_t length;
    unsigned char payload[MAX_PAYLOAD];
};

static bool encode_frame(const unsigned char *payload, size_t count,
                         unsigned char *dest, size_t capacity,
                         size_t *written)
{
    if (dest == nullptr || written == nullptr ||
        count > MAX_PAYLOAD || (count != 0 && payload == nullptr) ||
        capacity < FRAME_OVERHEAD + count)
        return false;

    unsigned char candidate[MAX_PAYLOAD + FRAME_OVERHEAD] = {};
    candidate[0] = 0xa5;
    candidate[1] = 1;
    candidate[2] = (unsigned char)count;
    candidate[3] = 0;
    if (count != 0)
        memcpy(candidate + 4, payload, count);

    unsigned char checksum = 0;
    for (size_t i = 0; i < 4 + count; ++i)
        checksum ^= candidate[i];
    candidate[4 + count] = checksum;

    memcpy(dest, candidate, FRAME_OVERHEAD + count);
    *written = FRAME_OVERHEAD + count;
    return true;
}

static bool decode_frame(const unsigned char *bytes, size_t length,
                         struct packet *out)
{
    if (bytes == nullptr || out == nullptr || length < FRAME_OVERHEAD ||
        bytes[0] != 0xa5 || bytes[1] != 1)
        return false;

    size_t count = (size_t)bytes[2] | ((size_t)bytes[3] << 8);
    if (count > MAX_PAYLOAD || length != FRAME_OVERHEAD + count)
        return false;

    unsigned char checksum = 0;
    for (size_t i = 0; i + 1 < length; ++i)
        checksum ^= bytes[i];
    if (checksum != bytes[length - 1])
        return false;

    struct packet candidate = {};
    candidate.length = count;
    if (count != 0)
        memcpy(candidate.payload, bytes + 4, count);
    *out = candidate;
    return true;
}

int main(void)
{
    unsigned char frame[MAX_PAYLOAD + FRAME_OVERHEAD] = {};
    unsigned char payload[MAX_PAYLOAD];
    for (size_t i = 0; i < sizeof payload; ++i)
        payload[i] = (unsigned char)(i + 1);

    struct packet result = {.length = 999};
    size_t written = 888;
    if (!encode_frame(nullptr, 0, frame, sizeof frame, &written) ||
        written != FRAME_OVERHEAD ||
        !decode_frame(frame, written, &result) || result.length != 0)
        return 1;

    if (!encode_frame(payload, sizeof payload, frame, sizeof frame, &written) ||
        written != sizeof frame ||
        !decode_frame(frame, written, &result) ||
        result.length != sizeof payload ||
        memcmp(result.payload, payload, sizeof payload) != 0)
        return 1;

    if (encode_frame(payload, sizeof payload, frame, sizeof frame - 1, &written) ||
        written != sizeof frame)
        return 1;

    unsigned char original = frame[6];
    frame[6] ^= 1;
    if (decode_frame(frame, sizeof frame, &result) ||
        result.length != sizeof payload)
        return 1;
    frame[6] = original;

    if (decode_frame(frame, sizeof frame - 1, &result) ||
        result.length != sizeof payload)
        return 1;

    if (encode_frame(nullptr, 1, frame, sizeof frame, &written))
        return 1;

    puts("frame_codec: OK");
    return 0;
}
