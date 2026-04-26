[org 0x7C00]
bits 16

STAGE2_SEG        equ 0x0800
STAGE2_OFF        equ 0x0000

STAGE2_SECTORS equ 8

SECTORS_PER_TRACK equ 18
HEADS_PER_CYLINDER equ 2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFFFF

    mov [boot_drive], dl

    ; debug: '1'
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:0], 0x0731

    ; load stage 2 to 0x0800:0000 = 0x00008000
    mov ax, STAGE2_SEG
    mov es, ax
    xor bx, bx

    mov byte [curr_sector], 2
    mov byte [curr_head], 0
    mov byte [curr_cylinder], 0
    mov word [sectors_left], STAGE2_SECTORS

.load_stage2:
    cmp word [sectors_left], 0
    je .done

    mov ah, 0x02
    mov al, 0x01
    mov ch, [curr_cylinder]
    mov cl, [curr_sector]
    mov dh, [curr_head]
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    add bx, 512
    dec word [sectors_left]

    inc byte [curr_sector]
    cmp byte [curr_sector], SECTORS_PER_TRACK + 1
    jne .load_stage2

    mov byte [curr_sector], 1
    inc byte [curr_head]
    cmp byte [curr_head], HEADS_PER_CYLINDER
    jne .load_stage2

    mov byte [curr_head], 0
    inc byte [curr_cylinder]
    jmp .load_stage2

.done:
    ; pass boot drive in DL
    mov dl, [boot_drive]

    ; far jump to stage 2
    jmp STAGE2_SEG:STAGE2_OFF

disk_error:
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:2], 0x0745   ; 'E'
.hang:
    cli
    hlt
    jmp .hang

boot_drive    db 0
curr_sector   db 0
curr_head     db 0
curr_cylinder db 0
sectors_left  dw 0

times 510-($-$$) db 0
dw 0xAA55