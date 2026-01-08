CC = gcc
CFLAGS = -Wall -Wextra -Isrc -DDESKTOP_BUILD

SRC = src/arm_emulator.c src/comm.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/main.c tests/testcase.c

.PHONY: all lib test clean

all: test_emulator

lib: libarm_emulator.a

libarm_emulator.a: $(OBJ)
	$(AR) rcs $@ $^

test_emulator: $(SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: test_emulator
	./test_emulator

clean:
	rm -f test_emulator libarm_emulator.a src/*.o
