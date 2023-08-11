NAME = chibi

INC := ./include
src := ./src/*.c

all:
	cc -g3 ${src} -I ${INC} 

