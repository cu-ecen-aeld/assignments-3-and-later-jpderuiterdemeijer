CFLAGS = -g -Wall -Werror

all: default

ifndef CC
	CC=$(CROSS_COMPILE)gcc
endif

default: aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f aesdsocket
	rm -f *.o
