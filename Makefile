CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

chibicc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: chibicc
	gcc -S test.c
	./test.sh

clean:
	rm -f chibicc *.o *~ tmp*

