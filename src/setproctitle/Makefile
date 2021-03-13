CC=gcc
OBJ=$(subst .c,.o,$(wildcard *.c))
EXEC=./a.out

# Add whatever MACRO here, that you want to pass to gcc via Makefile comand line, like "make PRSETNAME_ASWELL=y"
MACRO_LIST = PRSETNAME ARGV0_NOCHANGE ADD_PREFIX


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
	@show_test_result() { \
		cat0() { echo -en cat $$1 '\t'; cat $$1 | wc -c; cat -A $$1; echo -e "\n"; }; \
		cat1() { echo cat $$1; cat $$1; echo; }; \
		echo ---------- TEST RESULT ----------; \
		echo -en $(EXEC) '\t'; echo -en $(EXEC)'\x0' | wc -c; echo; \
		cat0 /proc/$$!/cmdline; \
		cat1 /proc/$$!/comm; \
		cat1 /proc/$$!/stat; \
		cat0 /proc/$$!/environ; \
		cat0 /proc/$$!/task/$$!/cmdline; \
		cat1 /proc/$$!/task/$$!/comm; \
		cat1 /proc/$$!/task/$$!/stat; \
		cat0 /proc/$$!/task/$$!/environ; \
		ps -o pid,tty,time,comm=COMM,cmd=CMDLINE $$!; \
	} >&2; \
	$(EXEC) & \
	sleep 0.01; : wait a sec to let EXEC print first if any; \
	show_test_result
