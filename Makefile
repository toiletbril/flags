.PHONY: klee examples clean

BC_FLAGS := -g3 -O1 -emit-llvm -c

%.bc: %.c
	clang $(BC_FLAGS) $< -o $@

BC_OUTPUT := klee_test.bc

KLEE_MAX_TIME := 60min
KLEE_MAX_MEMORY:= 16384
SYM_ARGS := 0 64 64

KLEE_FLAGS := --only-output-states-covering-new --optimize --max-time=$(KLEE_MAX_TIME) --max-memory=$(KLEE_MAX_MEMORY) --posix-runtime --libc=uclibc --ubsan-runtime

klee: $(BC_OUTPUT)
	klee $(KLEE_FLAGS) $(BC_OUTPUT) --sym-args $(SYM_ARGS)

CC := clang
CFLAGS := -g3 -Wall -Wextra -Wconversion -Wdouble-promotion -Werror -std=c99 -fsanitize=address

%: %.c
	$(CC) $(CFLAGS) $< -o $@

examples: example

FILES_TO_CLEAN := example *.bc klee-last klee-out-*

clean:
	rm -rf $(FILES_TO_CLEAN)
