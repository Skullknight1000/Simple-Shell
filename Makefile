CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
# -std=c11 hides POSIX/GNU functions such as getline(), strdup(), and strsep()
# unless a feature macro is defined. Keep this even if you change CFLAGS.
CPPFLAGS += -D_GNU_SOURCE
LDFLAGS ?=

TARGET := shell
OBJECTS := shell.o utils.o
TEST_TARGET := tests/test_utils

.PHONY: all build clean test

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

shell.o: shell.c utils.h
utils.o: utils.c utils.h

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_utils.c utils.c utils.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_utils.c utils.c -o $@

clean:
	$(RM) $(TARGET) $(OBJECTS) $(TEST_TARGET)
