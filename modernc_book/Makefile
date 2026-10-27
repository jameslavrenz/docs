CC ?= cc
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2
.PHONY: all clean check
all: hello digits parse_unsigned byte_decode word_count checked_add ring_buffer checked_sum vector file_copy word_index packet_decode byte_codec checked_interval spsc_queue callbacks utf8_decode compile_time c23_basics slices vector_checked frame_codec callback_sink modules_demo arena_bytes parse_signed rotate_bits line_reader thread_counter
line_reader: examples/line_reader.c
	$(CC) $(CFLAGS) $< -o $@
thread_counter: examples/thread_counter.c
	$(CC) $(CFLAGS) -pthread $< -o $@
arena_bytes: examples/arena_bytes.c
	$(CC) $(CFLAGS) $< -o $@
parse_signed: examples/parse_signed.c
	$(CC) $(CFLAGS) $< -o $@
rotate_bits: examples/rotate_bits.c
	$(CC) $(CFLAGS) $< -o $@
frame_codec: examples/frame_codec.c
	$(CC) $(CFLAGS) $< -o $@
callback_sink: examples/callback_sink.c
	$(CC) $(CFLAGS) $< -o $@
modules_demo: examples/modules/main.c examples/modules/checksum.c examples/modules/checksum.h
	$(CC) $(CFLAGS) examples/modules/main.c examples/modules/checksum.c -o $@
slices: examples/slices.c
	$(CC) $(CFLAGS) $< -o $@
vector_checked: examples/vector_checked.c
	$(CC) $(CFLAGS) $< -o $@
c23_basics: examples/c23_basics.c
	$(CC) $(CFLAGS) $< -o $@
compile_time: examples/compile_time.c
	$(CC) $(CFLAGS) $< -o $@
utf8_decode: examples/utf8_decode.c
	$(CC) $(CFLAGS) $< -o $@
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
	rm -f hello digits parse_unsigned byte_decode word_count checked_add ring_buffer checked_sum vector file_copy word_index packet_decode byte_codec checked_interval spsc_queue callbacks utf8_decode compile_time c23_basics slices vector_checked frame_codec callback_sink modules_demo arena_bytes parse_signed rotate_bits line_reader thread_counter
