# SPDX-License-Identifier: MIT
# Copyright (c) Thomas Makin 2025

.section .header, "a"
.align 4
.global _start
_start:
    .word main               # jump0
    .word 0x000BAC07         # Magic number (4 bytes)
    .word 0x00000000         # cksum (4 bytes), replaced at build time
    .word 0x00000200         # size (4 bytes), replaced at build time
    .dword 0x80200000        # runaddr (8 bytes)
    .word 0x00000000         # rsvd1 (4 bytes)
    .word 0x00000000         # rsvd2 (4 bytes)

.section .text.kernel
main:
        lla s0, tjm
        call _print
        call _reset
        ret

# bad print func
# expects base addr of string in t0
_print:
        # set counter to 0
        add t0, s0, zero
print_loop:
        li a7, 0x1 # SBI_EXT_0_1_CONSOLE_PUTCHAR
        lb a0, (t0)
        beqz a0, done
        ecall
        addi t0, t0, 0x1
        j print_loop
done:
        ret

# reset function
_reset:
        #li a7, 0x53525354 # extid SBI_EXT_SRST
        #li a6, 0x0 # funcid SBI_EXT_SRST_RESET
        #li a0, 0x1 # SBI_SRST_RESET_TYPE_COLD_REBOOT
        #li a1, 0x0 # SBI_SRST_RESET_REASON_NONE
        #ecall

        li a7, 0x8 # SBI_EXT_0_1_SHUTDOWN
        ecall

        # debug
        lla s0, debug
        call _print
        ret

.section .rodata
tjm:
        .incbin "tjm.txt"
        .byte 0
debug:
        .string "debug"
