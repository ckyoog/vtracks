
CC = gcc
CFLAGS = -O2 -Wall

all: a.out

a.out: in6_addr_to_64base.o main.o
	$(CC) $^ -o $@

clean:
	rm -f a.out *.o
