NAME = chibi

INC := ./include
src := ./src/*.c

all:
	cc ${src} -I ${INC}

