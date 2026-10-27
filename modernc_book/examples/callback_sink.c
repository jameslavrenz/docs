#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef size_t (*sink_write_fn)(void *, const unsigned char *, size_t);

struct byte_sink {
    sink_write_fn write;
    void *context;
};

static bool write_all(struct byte_sink sink,
                      const unsigned char *data, size_t count)
{
    if (sink.write == nullptr || (count != 0 && data == nullptr))
        return false;

    size_t offset = 0;
    while (offset < count) {
        size_t remaining = count - offset;
        size_t accepted = sink.write(sink.context, data + offset, remaining);
        if (accepted == 0 || accepted > remaining)
            return false;
        offset += accepted;
    }
    return true;
}

enum { STORAGE_CAPACITY = 32 };
struct fake_sink {
    unsigned char stored[STORAGE_CAPACITY];
    size_t used;
    size_t per_call;
    size_t calls;
};

static size_t fake_write(void *context,
                         const unsigned char *data, size_t count)
{
    struct fake_sink *sink = context;
    ++sink->calls;
    if (sink->used > STORAGE_CAPACITY)
        return 0;

    size_t available = STORAGE_CAPACITY - sink->used;
    size_t accepted = count;
    if (accepted > sink->per_call)
        accepted = sink->per_call;
    if (accepted > available)
        accepted = available;

    for (size_t i = 0; i < accepted; ++i)
        sink->stored[sink->used + i] = data[i];
    sink->used += accepted;
    return accepted;
}

int main(void)
{
    const unsigned char message[] = {'h','e','l','l','o'};
    struct fake_sink fake = {.per_call = 2};
    struct byte_sink sink = {.write = fake_write, .context = &fake};

    if (!write_all(sink, message, sizeof message) ||
        fake.used != sizeof message || fake.calls != 3)
        return 1;
    for (size_t i = 0; i < sizeof message; ++i)
        if (fake.stored[i] != message[i])
            return 1;

    fake.per_call = 0;
    size_t before = fake.used;
    if (write_all(sink, message, sizeof message) || fake.used != before)
        return 1;
    if (!write_all(sink, nullptr, 0))
        return 1;
    if (write_all((struct byte_sink){}, message, sizeof message))
        return 1;

    puts("callback_sink: OK");
    return 0;
}
