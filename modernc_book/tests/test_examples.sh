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
[ "$(./byte_decode)" = 'decoded: 12345678' ]
printf '%s\n' 'All example checks passed'
