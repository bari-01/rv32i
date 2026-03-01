CC      := gcc
CFLAGS  := -Wall -O0 -Wextra -std=c17
LDFLAGS :=

TARGET := main

SRCS := src/main.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)
	./main

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

.PHONY: all clean re
