#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./chibicc "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 3 '1 + 2'
assert 1 '2 - 1'
assert 2 '1 - 2 + 3'
assert 0 '1 - 2 + 1'
assert 10 '10 -      10    +   10'
echo OK
