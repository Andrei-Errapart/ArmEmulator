CC = gcc
CFLAGS = -Wall -Wextra -Isrc -DDESKTOP_BUILD

SRC = src/arm_emulator.c src/comm.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/main.c tests/testcase.c
EXAMPLES = examples/basic examples/callbacks examples/debug examples/lpc1114

.PHONY: all lib test examples clean

all: test_emulator

lib: libarm_emulator.a

libarm_emulator.a: $(OBJ)
	$(AR) rcs $@ $^

test_emulator: $(SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: test_emulator
	./test_emulator

examples: $(EXAMPLES)

examples/%: examples/%.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_emulator libarm_emulator.a src/*.o $(EXAMPLES)
