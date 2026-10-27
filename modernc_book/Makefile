CC ?= cc
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2
.PHONY: all clean check
all: hello digits
hello: examples/hello.c
	$(CC) $(CFLAGS) $< -o $@
digits: examples/digits.c
	$(CC) $(CFLAGS) $< -o $@
check: all
	@test "$$(./hello)" = "hello, world"
	@test "$$(printf '113\n' | ./digits | grep '^1: 2')" 
clean:
	rm -f hello digits
