CROSS ?= riscv64-unknown-linux-musl-
CC = $(CROSS)gcc
CFLAGS += -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
CFLAGS += -O3 -g3 -Wall -Wextra -static -flto -fanalyzer

all:
	$(CC) $(CFLAGS) vendortest.c -o vendortest

clean:
	rm -fv vendortest *.out
