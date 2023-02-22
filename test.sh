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
assert 0 '{ return 0; }'
assert 42 '{ return 42; }'
assert 21 '{ return 5+20-4; }'
assert 41 '{  return 12 + 34 - 5 ; }'
assert 47 '{ return 5+6*7; }'
assert 15 '{ return 5*(9-6); }'
assert 4 '{ return (3+5)/2; }'
assert 10 '{ return -10+20; }'
assert 10 '{ return - -10; }'
assert 10 '{ return - - +10; }'

assert 0 '{ return 0==1; }'
assert 1 '{ return 42==42; }'
assert 1 '{ return 0!=1; }'
assert 0 '{ return 42!=42; }'

assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'

assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'

assert 3 '{ int a=3; return a; }'
assert 8 '{int  a=3; int z=5; return a+z; }'

assert 3 '{int a=3; return a; }'
assert 8 '{int a=3;int z=5; return a+z; }'
assert 6 '{int a=3,b=3; return a+b; }'
assert 3 '{int foo=3; return foo; }'
assert 8 '{ int foo123=3;int bar=5; return foo123+bar; }'

assert 3 '{1;2;return 3;}'
assert 2 '{1;{return 2;}}'
assert 3 '{1; {2;} return 3;}'

assert 3 '{if (0) return 2; return 3;}'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 3 '{int x=3; return *&x; }'
assert 8 '{int a=2,b=4,c=6,d=8,e=10; return d;}'
assert 3 '{int x=3;int *y=&x;int **z=&y; return **z; }'
assert 3 '{int a=3;int *b=&a;int **c=&b;int ***d=&c;int ****e=&d;  ****e; }'
assert 5 '{int x=3;int y=5; return *(&x+1); }'
assert 3 '{int x=3;int y=5; return *(&y-1); }'
assert 5 '{int x=3;int *y=&x; *y=5; return x; }'
assert 7 '{int x=3;int y=5; *(&x+1)=7; return y; }'
assert 7 '{int  x=3;int y=5; *(&y-1)=7; return x; }'
assert 10 '{int x=1;int y=3;int z= 5;  *(&z -2) = 10; return x;}'

echo OK
