# SPDX-License-Identifier: MIT
# Copyright (c) Thomas Makin 2025

# default for standalone comp
CROSS_COMPILE ?= riscv64-unknown-elf-
AS ?= as
LD ?= ld
OBJCOPY ?= objcopy

# cvitek build system sets this for unknown elf triplet
ifneq ($(CROSS_COMPILE_GLIBC_RISCV64),)
CROSS_COMPILE := $(CROSS_COMPILE_GLIBC_RISCV64)
endif

kernel.bin: kernel.S
	$(CROSS_COMPILE)$(AS) -o kernel.o kernel.S
	$(CROSS_COMPILE)$(LD) -T test.ld -o kernel.elf kernel.o
	$(CROSS_COMPILE)$(OBJCOPY) -O binary kernel.elf $@

clean:
	@rm -f kernel.o
	@rm -f kernel.elf
	@rm -f kernel.bin
	@echo "Test kernel artifacts removed"
