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
        #call _print
        #call _reset
        ret

.section .rodata
text:
        .incbin "text.txt"
        .byte 0
debug:
        .string "debug"
