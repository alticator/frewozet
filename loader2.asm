[org 0x8000]
bits 16

TEMP_KERNEL_SEG      equ 0x1000
TEMP_KERNEL_ADDR     equ 0x00010000
KERNEL_PHYS_ADDR     equ 0x00100000
KERNEL_VIRT_BASE     equ 0xC0000000
KERNEL_VIRT_ENTRY    equ 0xC0100000

SECTORS_PER_TRACK    equ 18
HEADS_PER_CYLINDER   equ 2

E820_INFO_ADDR       equ 0x0500
E820_MAX_ENTRIES     equ 32
E820_SIG             equ 0x324F3845

PAGE_DIR_PHYS        equ 0x00009000
PAGE_TABLE0_PHYS     equ 0x0000A000

CODE_SEL             equ 0x08
DATA_SEL             equ 0x10

%include "kernel_sectors.inc"
STAGE2_SECTORS equ 8

loader2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFF00

    mov [boot_drive], dl

    ; debug: '2'
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:0], 0x0732

    call collect_e820

    ; load kernel temporarily to 0x1000:0000 = physical 0x00010000
    mov ax, TEMP_KERNEL_SEG
    mov es, ax
    xor bx, bx

    ; kernel starts right after stage2
    mov ax, STAGE2_SECTORS
    add ax, 2                  ; stage1 sector + stage2 sectors
    mov [kernel_lba_start], ax

    call lba_to_chs_from_kernel_start

    mov word [sectors_left], KERNEL_SECTORS

.load_kernel:
    cmp word [sectors_left], 0
    je .load_done

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
    jne .load_kernel

    mov byte [curr_sector], 1
    inc byte [curr_head]
    cmp byte [curr_head], HEADS_PER_CYLINDER
    jne .load_kernel

    mov byte [curr_head], 0
    inc byte [curr_cylinder]
    jmp .load_kernel

.load_done:
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:2], 0x074B   ; 'K'

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:protected_mode

disk_error:
    mov ax, 0xB800
    mov gs, ax
    mov word [gs:4], 0x0745   ; 'E'
.hang:
    cli
    hlt
    jmp .hang

; Convert linear sector index (1-based disk sectors) to floppy CHS.
; Input: [kernel_lba_start] = absolute sector number on disk, where sector 1 is boot sector.
; Output: curr_cylinder, curr_head, curr_sector
lba_to_chs_from_kernel_start:
    ; For floppy:
    ; sector numbers on disk:
    ; 1..18 = cyl0 head0
    ; 19..36 = cyl0 head1
    ; etc.
    ;
    ; BIOS CHS sector is 1-based.
    ;
    ; Convert absolute sector number to zero-based logical sector index.
    mov ax, [kernel_lba_start]
    dec ax

    xor dx, dx
    mov cx, SECTORS_PER_TRACK
    div cx                     ; AX = track_sector_group, DX = sector_in_track
    mov byte [curr_sector], dl
    inc byte [curr_sector]

    xor dx, dx
    mov cx, HEADS_PER_CYLINDER
    div cx                     ; AX = cylinder, DX = head

    mov byte [curr_cylinder], al
    mov byte [curr_head], dl
    ret

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

boot_drive       db 0
curr_sector      db 0
curr_head        db 0
curr_cylinder    db 0
sectors_left     dw 0
kernel_lba_start dw 0

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

    mov word [0xB8004], 0x0750   ; 'P'

    ; Copy kernel from temp low address to final physical 1 MiB
    mov esi, TEMP_KERNEL_ADDR
    mov edi, KERNEL_PHYS_ADDR
    mov ecx, KERNEL_SECTORS
    shl ecx, 9
    rep movsb

    ; clear page directory
    mov edi, PAGE_DIR_PHYS
    mov ecx, 1024
    xor eax, eax
    rep stosd

    ; clear page table 0
    mov edi, PAGE_TABLE0_PHYS
    mov ecx, 1024
    xor eax, eax
    rep stosd

    ; map first 4 MiB identity and again at 0xC0000000
    mov edi, PAGE_TABLE0_PHYS
    xor ebx, ebx
    mov ecx, 1024
.map_loop:
    mov eax, ebx
    or eax, 0x003
    mov [edi], eax
    add ebx, 0x1000
    add edi, 4
    loop .map_loop

    mov eax, PAGE_TABLE0_PHYS
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 0*4], eax
    mov [PAGE_DIR_PHYS + 768*4], eax

    mov eax, PAGE_DIR_PHYS
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp CODE_SEL:higher_half_entry

higher_half_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0xC0090000

    mov word [0xC00B8006], 0x0748   ; 'H'

    mov eax, KERNEL_VIRT_ENTRY
    jmp eax