bits 32
section .text.start

global _start
extern kmain
extern _bss_start
extern _bss_end

_start:
    cld

    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call kmain

.hang:
    cli
    hlt
    jmp .hang