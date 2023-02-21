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

assert 3 '1 + 2;'
assert 1 '2 - 1;'
assert 2 '1 - 2 + 3;'
assert 0 '1 - 2 + 1;'
assert 10 '10 -      10    +   10;'
assert 6   '3*2*1;'
assert 0  '5 * 0;'
assert 4   '(2 * 4 * 5) / 10;' 
assert 0  '(+2 - 2);'
assert 0 '(-5 +5);'
assert 10 '- - + 10;'
assert 10 '- - -  - 10;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 10 'x=10; x;'
assert 8  'a=3; z=5; a+z;'
assert 20 'foo=5; bar = 15; foo+bar;'
assert 100 'foo=10; bar =10; foo*bar;'
echo OK
