CC ?= cc
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2
.PHONY: all clean check
all: hello digits parse_unsigned byte_decode word_count checked_add ring_buffer checked_sum vector file_copy word_index packet_decode byte_codec checked_interval spsc_queue callbacks
callbacks: examples/callbacks.c
	$(CC) $(CFLAGS) $< -o $@
spsc_queue: examples/spsc_queue.c
	$(CC) $(CFLAGS) $< -o $@
checked_interval: examples/checked_interval.c
	$(CC) $(CFLAGS) $< -o $@
byte_codec: examples/byte_codec.c
	$(CC) $(CFLAGS) $< -o $@
packet_decode: examples/packet_decode.c
	$(CC) $(CFLAGS) $< -o $@
word_index: examples/word_index.c
	$(CC) $(CFLAGS) $< -o $@
vector: examples/vector.c
	$(CC) $(CFLAGS) $< -o $@
file_copy: examples/file_copy.c
	$(CC) $(CFLAGS) $< -o $@
ring_buffer: examples/ring_buffer.c
	$(CC) $(CFLAGS) $< -o $@
checked_sum: examples/checked_sum.c
	$(CC) $(CFLAGS) $< -o $@
word_count: examples/word_count.c
	$(CC) $(CFLAGS) $< -o $@
checked_add: examples/checked_add.c
	$(CC) $(CFLAGS) $< -o $@
parse_unsigned: examples/parse_unsigned.c
	$(CC) $(CFLAGS) $< -o $@
byte_decode: examples/byte_decode.c
	$(CC) $(CFLAGS) $< -o $@
hello: examples/hello.c
	$(CC) $(CFLAGS) $< -o $@
digits: examples/digits.c
	$(CC) $(CFLAGS) $< -o $@
check: all
	@sh tests/test_examples.sh
clean:
	rm -f hello digits parse_unsigned byte_decode word_count checked_add ring_buffer checked_sum vector file_copy word_index packet_decode byte_codec
