CC=gcc
OBJ=$(subst .c,.o,$(wildcard *.c))
EXEC=a.out

# Add whatever MACRO here, that you want to pass to gcc via Makefile comand line, like "make PRSETNAME_ASWELL=y"
MACRO_LIST = PRSETNAME ARGV0_NOCHANGE


# "$(call build-CFLAGS,XXXXXX)" is equivalent to "CFLAGS-$(XXXXXX) += -DXXXXXX"
build-CFLAGS = $(eval CFLAGS-$$($1) += -D$1)
$(foreach m,$(MACRO_LIST),$(call build-CFLAGS,$m))

CFLAGS=$(CFLAGS-y)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ)

clean:
	rm -f $(EXEC) *.o

test: all
	@./$(EXEC) & \
	  cat /proc/$$!/cmdline \
	      /proc/$$!/comm \
	      /proc/$$!/stat \
	      /proc/$$!/task/$$!/cmdline \
	      /proc/$$!/task/$$!/comm \
	      /proc/$$!/task/$$!/stat \
	  | tr '\0' '\n'; \
	  ps -o pid,tty,time,comm=COMM,cmd=CMDLINE;
