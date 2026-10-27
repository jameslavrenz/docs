CC ?= cc
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2
.PHONY: all clean check
all: hello digits parse_unsigned byte_decode word_count checked_add
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
	@./parse_unsigned >/dev/null
	@./byte_decode >/dev/null
	@./checked_add >/dev/null
	@test "$(printf 'a b c' | ./word_count)" = '3'
	@test "$(./hello)" = "hello, world"
	@test "$$(printf '113\n' | ./digits | grep '^1: 2')" 
clean:
	rm -f hello digits parse_unsigned byte_decode word_count checked_add
