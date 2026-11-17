CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lreadline
TARGET = bin/myshell
SOURCES = src/main.c src/shell.c src/execute.c
OBJECTS = $(SOURCES:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/*.o bin/myshell

.PHONY: all clean
