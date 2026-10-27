#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
make all
[ "$(./hello)" = 'hello, world' ]
[ "$(printf '113\n' | ./digits | grep '^1: 2$')" = '1: 2' ]
[ "$(printf 'one   two\nthree' | ./word_count)" = '3' ]
[ "$(printf '' | ./word_count)" = '0' ]
[ "$(./parse_unsigned)" = 'parse_unsigned: OK' ]
[ "$(./checked_add)" = 'checked_add: OK' ]
[ "$(./ring_buffer)" = 'ring_buffer: OK' ]
[ "$(./checked_sum)" = 'checked_sum: OK' ]
[ "$(./vector)" = 'vector: OK' ]
[ "$(./packet_decode)" = 'packet_decode: OK' ]
[ "$(./byte_codec)" = 'byte_codec: OK' ]
[ "$(./checked_interval)" = 'checked_interval: OK' ]
[ "$(printf 'C23 binary copy' | ./file_copy)" = 'C23 binary copy' ]
[ "$(./byte_decode)" = 'decoded: 12345678' ]
sh tests/test_word_index.sh
printf '%s\n' 'All example checks passed'
