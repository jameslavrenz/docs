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
echo 'word_index: OK'
