# CS:APP ch. 3 examples — see the C-to-assembly guide in the wiki.
#
#   make              # assembly listings (.s) for every example
#   make arith.s      # just one
#   make dis          # objdump disassembly of the compiled objects
#   make run          # build and run the ones with a main()
#   make canary       # stack protector on vs off, side by side
#   make clean

CC      ?= gcc
# -Og keeps the output readable: optimized enough to be realistic, not so
# much that the C structure disappears. -fno-asynchronous-unwind-tables
# drops the .cfi_* bookkeeping directives that bury short listings.
CFLAGS  ?= -Og -std=c17 -Wall -Wextra -fno-asynchronous-unwind-tables

SRCS  := arith.c arrays.c control.c fp.c overflow.c proc.c structs.c switch.c
ASMS  := $(SRCS:.c=.s)
OBJS  := $(SRCS:.c=.o)
RUNNABLE := structs

.PHONY: all dis run canary clean
all: $(ASMS)

%.s: %.c
	$(CC) $(CFLAGS) -S $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

dis: $(OBJS)
	@for o in $(OBJS); do \
	  echo "===== $$o ====="; \
	  objdump -d --no-show-raw-insn -M att $$o; \
	done

run: $(RUNNABLE)
	@./structs

structs: structs.c
	$(CC) -O2 -Wall -Wextra -o $@ $<

canary: overflow.c
	$(CC) $(CFLAGS) -S $< -o with-canary.s
	$(CC) $(CFLAGS) -fno-stack-protector -S $< -o without-canary.s
	-diff -u without-canary.s with-canary.s

clean:
	rm -f $(ASMS) $(OBJS) $(RUNNABLE) with-canary.s without-canary.s
