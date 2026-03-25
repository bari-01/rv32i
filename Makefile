CC      := gcc
CFLAGS  := -Wall -O2 -Wextra -std=c17
LDFLAGS :=

TARGET := main

SRCS := src/main.c src/guest.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test:
	riscv32-unknown-elf-as -march=rv32i test.s -o test.o
	riscv32-unknown-elf-ld test.o -o test.elf
	riscv32-unknown-elf-objcopy -O binary test.elf test.bin
	./main test.bin

re: clean all

.PHONY: all clean re
