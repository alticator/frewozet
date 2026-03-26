[org 0x7C00]
bits 16

KERNEL_LOAD_SEG  equ 0x1000
KERNEL_LOAD_ADDR equ 0x10000
KERNEL_SECTORS   equ 8

CODE_SEL equ 0x08
DATA_SEL equ 0x10

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFFFF

    mov [boot_drive], dl

    ; write 'A' to top-left in text mode
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:0], 0x0741

    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx
    xor si, si

.load_kernel:
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov dh, 0

    mov ax, si
    add al, 2
    mov cl, al

    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    add bx, 512
    inc si
    cmp si, KERNEL_SECTORS
    jl .load_kernel

    ; write 'B' after successful load
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:2], 0x0742

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:protected_mode

disk_error:
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:4], 0x0745      ; 'E'
.hang:
    cli
    hlt
    jmp .hang

boot_drive db 0

gdt_start:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

bits 32
protected_mode:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov word [0xB8004], 0x0750      ; 'P'

    jmp KERNEL_LOAD_ADDR

times 510-($-$$) db 0
dw 0xAA55