CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

com: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: com 
	./test.sh

clean:
	rm -f com *.o *~ tmp*

