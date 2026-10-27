#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
make word_index
actual=$(printf 'Pear apple pear BANANA apple\n' | ./word_index)
expected=$(printf 'apple 2\nbanana 1\npear 2')
[ "$actual" = "$expected" ]
[ -z "$(printf '' | ./word_index)" ]
if printf 'abcdefghijklmnopqrstuvwxyzabcdef\n' | ./word_index >/dev/null 2>&1; then
  echo 'expected overlong-word rejection' >&2
  exit 1
fi
# Exactly 31 characters fit; a final token without newline is accepted.
exact=$(printf 'abcdefghijklmnopqrstuvwxyzabcde' | ./word_index)
[ "$exact" = 'abcdefghijklmnopqrstuvwxyzabcde 1' ]
# Another 32-character token must be rejected without truncation.
if printf 'abcdefghijklmnopqrstuvwxyzabcdef' | ./word_index >/dev/null 2>&1; then
  echo 'expected 32-byte token rejection at EOF' >&2
  exit 1
fi
# Insertion at the front repeatedly must preserve sorted output.
reverse=$(printf 'z y x' | ./word_index)
expected_reverse=$(printf 'x 1\ny 1\nz 1')
[ "$reverse" = "$expected_reverse" ]
echo 'word_index: OK'
