CC=gcc
CFLAGS=-g -Wall -Wextra -Werror
LDLIBS=
OBJ=*o
EXE=timeout

all: timeout
.PHONY: all clean

timeout: timeout.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

debug: CFLAGS+= -DDEBUG
debug: timeout

clean:
	rm -f $(OBJ) $(EXE)
