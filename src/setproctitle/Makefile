CC=gcc
OBJ=$(subst .c,.o,$(wildcard *.c))
EXEC=a.out

CFLAGS-$(PRSETNAME_ASWELL) += -DPRSETNAME_ASWELL


CFLAGS=$(CFLAGS-y)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ)

clean:
	rm -f $(EXEC) *.o

run: all
	@./$(EXEC) & \
	  cat /proc/$$!/cmdline /proc/$$!/comm /proc/$$!/stat /proc/$$!/task/$$!/cmdline /proc/$$!/task/$$!/comm /proc/$$!/task/$$!/stat | tr '\0' '\n'; ps
