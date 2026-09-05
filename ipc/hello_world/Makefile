CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = hello_world

all: $(TARGET)

$(TARGET): hello_world.c
	$(CC) $(CFLAGS) -o $@ $^

install:
	mkdir -p rootfs/bin
	cp $(TARGET) rootfs/bin/

clean:
	rm -f $(TARGET)

.PHONY: all install clean