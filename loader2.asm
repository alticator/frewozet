[org 0x8000]
bits 16

TEMP_KERNEL_SEG      equ 0x1000
TEMP_KERNEL_ADDR     equ 0x00010000
KERNEL_PHYS_ADDR     equ 0x00100000
KERNEL_VIRT_BASE     equ 0xC0000000
KERNEL_VIRT_ENTRY    equ 0xC0100000

E820_INFO_ADDR       equ 0x0500
E820_MAX_ENTRIES     equ 32
E820_SIG             equ 0x324F3845

PAGE_DIR_PHYS        equ 0x00009000
PAGE_TABLE0_PHYS     equ 0x0000A000

CODE_SEL             equ 0x08
DATA_SEL             equ 0x10

%include "kernel_sectors.inc"
STAGE2_SECTORS equ 8
KERNEL_LBA_START equ 9

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
    call enable_a20

    ; load kernel temporarily to 0x1000:0000 = physical 0x00010000 using LBA
    mov ax, TEMP_KERNEL_SEG
    mov es, ax
    xor bx, bx

    mov dword [dap_lba], KERNEL_LBA_START
    mov word [dap_seg], TEMP_KERNEL_SEG
    mov word [dap_off], 0
    mov word [dap_count], KERNEL_SECTORS

    call lba_read
    jc disk_error

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

; LBA Read: Read sectors from disk using int 0x13 AH=0x42 (Extended Read)
; Input: [dap_lba], [dap_count], [dap_seg], [dap_off], [boot_drive]
; Output: CF clear on success
lba_read:
    pushad
    push es
    
    mov dl, [boot_drive]
    mov ah, 0x42
    mov si, dap_packet
    int 0x13
    
    pop es
    popad
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

enable_a20:
    in al, 0x92
    or al, 00000010b
    out 0x92, al
    ret

; DAP (Disk Address Packet) for int 0x13 AH=0x42
dap_packet:
    db 0x10                ; DAP size (16 bytes)
    db 0x00                ; reserved
dap_count:
    dw 0                   ; number of sectors to read
dap_off:
    dw 0                   ; destination offset
dap_seg:
    dw 0                   ; destination segment
dap_lba:
    dd 0                   ; LBA (logical block address)
    dd 0                   ; upper 32 bits of LBA (for 64-bit addressing)

boot_drive       db 0

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

    ; clear page table 0 through 15 (for 64 MiB coverage)
    mov edi, PAGE_TABLE0_PHYS
    mov ecx, 16384              ; 16 tables * 1024 entries each
    xor eax, eax
    rep stosd

    ; map first 64 MiB identity and again at 0xC0000000
    ; Page table 0 -> Directory entries 0 and 768
    mov eax, PAGE_TABLE0_PHYS
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 0*4], eax
    mov [PAGE_DIR_PHYS + 768*4], eax

    ; Page table 1 -> Directory entries 1 and 769
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x1000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 1*4], eax
    mov [PAGE_DIR_PHYS + 769*4], eax

    ; Page table 2 -> Directory entries 2 and 770
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x2000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 2*4], eax
    mov [PAGE_DIR_PHYS + 770*4], eax

    ; Page table 3 -> Directory entries 3 and 771
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x3000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 3*4], eax
    mov [PAGE_DIR_PHYS + 771*4], eax

    ; Page table 4 -> Directory entries 4 and 772
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x4000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 4*4], eax
    mov [PAGE_DIR_PHYS + 772*4], eax

    ; Page table 5 -> Directory entries 5 and 773
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x5000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 5*4], eax
    mov [PAGE_DIR_PHYS + 773*4], eax

    ; Page table 6 -> Directory entries 6 and 774
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x6000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 6*4], eax
    mov [PAGE_DIR_PHYS + 774*4], eax

    ; Page table 7 -> Directory entries 7 and 775
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x7000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 7*4], eax
    mov [PAGE_DIR_PHYS + 775*4], eax

    ; Page table 8 -> Directory entries 8 and 776
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x8000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 8*4], eax
    mov [PAGE_DIR_PHYS + 776*4], eax

    ; Page table 9 -> Directory entries 9 and 777
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0x9000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 9*4], eax
    mov [PAGE_DIR_PHYS + 777*4], eax

    ; Page table 10 -> Directory entries 10 and 778
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xA000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 10*4], eax
    mov [PAGE_DIR_PHYS + 778*4], eax

    ; Page table 11 -> Directory entries 11 and 779
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xB000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 11*4], eax
    mov [PAGE_DIR_PHYS + 779*4], eax

    ; Page table 12 -> Directory entries 12 and 780
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xC000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 12*4], eax
    mov [PAGE_DIR_PHYS + 780*4], eax

    ; Page table 13 -> Directory entries 13 and 781
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xD000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 13*4], eax
    mov [PAGE_DIR_PHYS + 781*4], eax

    ; Page table 14 -> Directory entries 14 and 782
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xE000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 14*4], eax
    mov [PAGE_DIR_PHYS + 782*4], eax

    ; Page table 15 -> Directory entries 15 and 783
    mov eax, PAGE_TABLE0_PHYS
    add eax, 0xF000
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 15*4], eax
    mov [PAGE_DIR_PHYS + 783*4], eax

    ; Populate all 16 page tables with identity mappings
    ; Each page table covers 4 MiB, so table N covers phys 0xN00000 to 0xN3FFFF
    xor ecx, ecx                ; table index
.populate_tables:
    cmp ecx, 16
    jge .done_populating

    mov eax, PAGE_TABLE0_PHYS
    mov edx, ecx
    shl edx, 12                 ; edx = ecx * 4096
    add eax, edx                ; eax = page table address for this table

    mov edi, eax                ; destination
    mov ebx, ecx
    shl ebx, 22                 ; ebx = ecx * 4194304 (4 MiB per table)
    mov edx, 1024               ; 1024 entries per table

.fill_entries:
    mov eax, ebx
    or eax, 0x003
    mov [edi], eax
    add ebx, 0x1000
    add edi, 4
    dec edx
    jnz .fill_entries

    inc ecx
    jmp .populate_tables

.done_populating:

    ; Set up recursive mapping
    mov eax, PAGE_DIR_PHYS
    or eax, 0x003
    mov [PAGE_DIR_PHYS + 1023*4], eax

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