bits 32
global _start
extern kmain

_start:
    call kmain

.hang:
    cli
    hlt
    jmp .hang