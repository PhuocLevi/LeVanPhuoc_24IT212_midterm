CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=gnu11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -O2 -g

TARGET = myls
SRCS = main.c options.c list.c format.c util.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET) ls

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

ls: $(TARGET)
	@ln -sf $(TARGET) ls

%.o: %.c options.h list.h format.h util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) ls

test: all
	@chmod +x tests/test_ls.sh
	@./tests/test_ls.sh
