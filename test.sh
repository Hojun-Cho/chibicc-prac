#!/bin/bash
assert() {
	expected="$1"
	input="$2"

	./chibicc "$input" > tmp.s || exit
	gcc -static -o tmp tmp.s test.s
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}
assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() {  return 12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() {int  a=3; int z=5; return a+z; }'

assert 3 'int main() {int a=3; return a; }'
assert 8 'int main() {int a=3;int z=5; return a+z; }'
assert 6 'int main() {int a=3,b=3; return a+b; }'
assert 3 'int main() {int foo=3; return foo; }'
assert 8 'int main() { int foo123=3;int bar=5; return foo123+bar; }'

assert 3 'int main() {1;2;return 3;}'
assert 2 'int main() {1;{return 2;}}'
assert 3 'int main() {1; {2;} return 3;}'

assert 3 'int main() {if (0) return 2; return 3;}'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'

assert 3 'int main() {int x=3; return *&x; }'
assert 8 'int main() {int a=2,b=4,c=6,d=8,e=10; return d;}'
assert 3 'int main() {int x=3;int *y=&x;int **z=&y; return **z; }'
assert 3 'int main() {int a=3;int *b=&a;int **c=&b;int ***d=&c;int ****e=&d;  ****e; }'
assert 5 'int main() {int x=3;int y=5; return *(&x+1); }'
assert 3 'int main() {int x=3;int y=5; return *(&y-1); }'
assert 5 'int main() {int x=3;int *y=&x; *y=5; return x; }'
assert 7 'int main() {int x=3;int y=5; *(&x+1)=7; return y; }'
assert 7 'int main() {int  x=3;int y=5; *(&y-1)=7; return x; }'
assert 10 'int main() {int x=1;int y=3;int z= 5;  *(&z -2) = 10; return x;}'

assert 2 'int main() {int x=2;{int x=3;} return x;}'
assert 3 'int main() {int x=2;{int x=3; return x;} return x;}'
assert 2 'int main() {int x=2; { int x=3; } { int y=4; return x; }}'

assert 3 'int main() {int x[2]; int *y=&x; *y=3; return *x;}'
assert 3 'int main() {int x[3]; int *y = x; *y = 3; return *x;}'

assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 0 'int main() {int x[2][3]; int *y=x; *y = 0; return **x;}'
assert 1 'int main() {int x[3]; x[2] =1; return x[2];}'
assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'
assert 4 'int main() {int x;return sizeof(x);}'
assert 8 'int main() {int x[2]; return sizeof(x);}'
assert 24 'int main() { int x[2][3]; return sizeof(x);}'
aasert 8 'int main() {int *x; return sizeof(x)}'
assert 0 'int ret_1() {return 1;} int ret_2() {return 2;} int main() {return 0;}'

assert 2 'int main() { int x=1; {int x=2; {int x= 3;} return x;}}'
assert 1 'int x,y,z;int main(){x = 0; y=1;z=2; return y;}'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'
assert 0 'int x,y[100],z[10]; int main() {y[49] = 0; return y[49];}'
assert 32 'int main() { return ret_32();}'
assert 64 'int main() {return ret_64();}'
assert 96 'int main() {return ret_32() + ret_64();}'
assert 3  'int main() {return add(1,2);}'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 1 'int main() { char x=1; return x; }'
assert 1 'int main() { char x=1; char y=2; return x; }'
assert 2 'int main() { char x=1; char y=2; return y; }'

assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'

assert 97 "int main() { char x = 'a'; return x;}"
assert 49 "int main() { char x = '1'; return x;}"
assert 49 "int main(){ int x=3; char y ='1'; int z = 6; return y;}"

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'
assert 10 "int main() {char x = '\n'; return x;}"
assert 92 "int main() {char x ='\\'; return x;}"

assert 97 "int main() {char *x = \"abcdef\"; return x[0];}"
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 2 'int main() { short x; return sizeof(x);}'
assert 4 'int main() { return sizeof(int);}'
assert 1 'int main() {return sizeof(char);}'
assert 1 'int main() {return sizeof((char));}'
assert 8 'int main() {return sizeof(long);}'
assert 8 'int main() {long x; return sizeof(x);}'
assert 10 'int main() {long x=1; long y=9; return x+y;}'

assert 100 'long temp() {return 100;} int main() {int x = temp(); return x;}'
assert 0 'int main() { void *x; return 0;}'

assert 4 'int main (){ struct {int a;} x; sizeof(x); }'
assert 5 'int main (){ struct {int a;char b;} x; sizeof(x); }'
assert 10 'int main(){struct {int a;char b; int c;} x; x.c = 10; return x.c;}'

assert 1 'int main() {struct temp{int a; int b;}; struct temp x; x.a = 1;return x.a;}'
assert 1 'int main() {struct temp{int a; char b; int c;}; struct temp x; x.b = 1;return x.b;}'
assert 1 'int main() {struct temp{int a; char b; int c;}; struct temp x[2]; x[1].b = 1; return x[1].b;}'

assert 3 'int main(){ struct {int a; int b;} x,y; x.a=3; y=x; return y.a;}'
echo OK

