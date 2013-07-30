CC=gcc
CFLAGS=

all: makestuff

OBJS=$(patsubst %.c,%.o,$(wildcard *c))

%.o : %.c
	$(CC) $(CFLAGS) -c $<

makestuff: $(OBJS)
	gcc $(CFLAGS) -o yabi $(OBJS)

clean:
	rm *.o
	rm yabi

.PHONY: all
