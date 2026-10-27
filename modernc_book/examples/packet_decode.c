#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* Packet: A5, version 01, little-endian 16-bit length, payload, XOR checksum.
   Maximum payload is 64 bytes. All output state remains unchanged on failure. */
enum { MAX_PAYLOAD = 64, HEADER_SIZE = 4 };
struct packet { unsigned char payload[MAX_PAYLOAD]; size_t length; };
static bool decode(const unsigned char *bytes, size_t size, struct packet *out)
{
    if (bytes == nullptr || out == nullptr || size < HEADER_SIZE + 1) return false;
    if (bytes[0] != 0xA5 || bytes[1] != 1) return false;
    size_t n = (size_t)bytes[2] | ((size_t)bytes[3] << 8);
    if (n > MAX_PAYLOAD || n > size - HEADER_SIZE - 1) return false;
    if (size != HEADER_SIZE + n + 1) return false;
    unsigned char checksum = 0;
    for (size_t i = 0; i < size - 1; ++i) checksum ^= bytes[i];
    if (checksum != bytes[size - 1]) return false;
    struct packet candidate = {};
    candidate.length = n;
    for (size_t i = 0; i < n; ++i) candidate.payload[i] = bytes[HEADER_SIZE + i];
    *out = candidate;
    return true;
}
int main(void)
{
    unsigned char valid[] = {0xA5, 1, 2, 0, 0x11, 0x22, 0};
    for (size_t i = 0; i < sizeof valid - 1; ++i) valid[6] ^= valid[i];
    struct packet p = {};
    if (!decode(valid, sizeof valid, &p) || p.length != 2 || p.payload[1] != 0x22) return 1;
    size_t old_length = p.length;
    valid[6] ^= 1;
    if (decode(valid, sizeof valid, &p) || p.length != old_length) return 1;
    if (decode(valid, 3, &p) || decode(nullptr, sizeof valid, &p)) return 1;
    puts("packet_decode: OK");
    return 0;
}
