CC=gcc
CFLAGS=

all: objs yabibin

objs:
	gcc -c yabid.c

yabibin:
	gcc -o yabi yabi.c yabid.o

clean:
	rm *.o
	rm yabi

.PHONY: all
