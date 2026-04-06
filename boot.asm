[org 0x7C00]
bits 16

KERNEL_LOAD_SEG  equ 0x1000
KERNEL_LOAD_ADDR equ 0x10000

%include "kernel_sectors.inc"

SECTORS_PER_TRACK equ 18
HEADS_PER_CYLINDER equ 2

E820_INFO_ADDR    equ 0x0500
E820_MAX_ENTRIES  equ 32
E820_SIG          equ 0x324F3845

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

    call collect_e820

    ; debug: A
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:0], 0x0741

    ; load kernel to 0x1000:0000 = physical 0x10000
    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx

    ; first kernel sector is right after boot sector:
    ; cylinder 0, head 0, sector 2
    mov byte [curr_sector], 2
    mov byte [curr_head], 0
    mov byte [curr_cylinder], 0
    mov word [sectors_left], KERNEL_SECTORS

load_kernel:
    cmp word [sectors_left], 0
    je load_done

    mov ah, 0x02          ; BIOS read sectors
    mov al, 0x01          ; read 1 sector
    mov ch, [curr_cylinder]
    mov cl, [curr_sector]
    mov dh, [curr_head]
    mov dl, [boot_drive]

    int 0x13
    jc disk_error

    add bx, 512
    dec word [sectors_left]

    ; advance CHS
    inc byte [curr_sector]
    cmp byte [curr_sector], SECTORS_PER_TRACK + 1
    jne load_kernel

    ; wrap sector, advance head
    mov byte [curr_sector], 1
    inc byte [curr_head]
    cmp byte [curr_head], HEADS_PER_CYLINDER
    jne load_kernel

    ; wrap head, advance cylinder
    mov byte [curr_head], 0
    inc byte [curr_cylinder]
    jmp load_kernel

load_done:
    ; debug: B
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:2], 0x0742

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:protected_mode

disk_error:
    ; debug: E
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:4], 0x0745

.hang:
    cli
    hlt
    jmp .hang

boot_drive    db 0
curr_sector   db 0
curr_head     db 0
curr_cylinder db 0
sectors_left  dw 0

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
    
collect_e820:
    pushad
    push es
    push di

    xor ax, ax
    mov es, ax

    mov di, E820_INFO_ADDR
    mov dword [es:di], E820_SIG
    mov word  [es:di+4], 0
    mov word  [es:di+6], 0

    mov di, E820_INFO_ADDR + 8
    xor ebx, ebx

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    mov dword [es:di+20], 1
    int 0x15
    jc .done

    cmp eax, 0x534D4150
    jne .done

    mov eax, [es:di+8]
    or  eax, [es:di+12]
    jz .skip_store

    inc word [es:E820_INFO_ADDR + 4]
    add di, 24

    cmp word [es:E820_INFO_ADDR + 4], E820_MAX_ENTRIES
    jae .done

.skip_store:
    test ebx, ebx
    jne .e820_loop

.done:
    pop di
    pop es
    popad
    ret

bits 32
protected_mode:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; debug: P
    mov word [0xB8004], 0x0750

    mov eax, KERNEL_LOAD_ADDR
    jmp eax

times 510-($-$$) db 0
dw 0xAA55