CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=gnu99 -O2 -g -Iinclude
TARGET = ls

SRCS = src/main.c \
       src/display.c \
       src/sort.c \
       src/traversal.c \
       src/util.c

OBJS = $(SRCS:src/%.c=obj/%.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj $(TARGET)

test: all
	@chmod +x tests/test_ls.sh
	@./tests/test_ls.sh
