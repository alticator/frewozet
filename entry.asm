bits 32
section .text.start
global _start
extern kmain

_start:
    mov word [0xB8006], 0x0753    ; 'S'
    cld
    call kmain

.hang:
    cli
    hlt
    jmp .hang