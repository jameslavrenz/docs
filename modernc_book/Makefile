CC ?= cc
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2
.PHONY: all clean check
all: hello digits parse_unsigned byte_decode
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
	@test "$(./hello)" = "hello, world"
	@test "$$(printf '113\n' | ./digits | grep '^1: 2')" 
clean:
	rm -f hello digits parse_unsigned byte_decode
