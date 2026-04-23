bits 32
section .text.start
global _start
extern kmain

_start:
    cld
    call kmain

.hang:
    cli
    hlt
    jmp .hang