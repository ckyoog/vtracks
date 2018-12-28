CC=gcc
OBJ=$(subst .c,.o,$(wildcard *.c))

all: $(OBJ)
	$(CC) $(OBJ)

clean:
	rm -f a.out *.o

run: all
	@./a.out & \
	  cat /proc/$$!/cmdline /proc/$$!/comm /proc/$$!/stat /proc/$$!/task/$$!/cmdline /proc/$$!/task/$$!/comm /proc/$$!/task/$$!/stat | tr '\0' '\n'; ps
