[org 0x7C00]
bits 16

STAGE2_SEG        equ 0x0800
STAGE2_OFF        equ 0x0000

STAGE2_SECTORS equ 8
STAGE2_LBA_START equ 1

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFFFF

    mov [boot_drive], dl

    call serial_init
    mov al, '1'
    call serial_putchar

    ; load stage 2 to 0x0800:0000 = 0x00008000 using LBA
    mov ax, STAGE2_SEG
    mov es, ax
    xor bx, bx

    mov dword [dap_lba], STAGE2_LBA_START
    mov word [dap_seg], STAGE2_SEG
    mov word [dap_off], 0
    mov word [dap_count], STAGE2_SECTORS

    call lba_read
    jc disk_error

.done:
    ; pass boot drive in DL
    mov dl, [boot_drive]

    ; far jump to stage 2
    jmp STAGE2_SEG:STAGE2_OFF

disk_error:
    mov al, 'E'
    call serial_putchar
.hang:
    cli
    hlt
    jmp .hang

serial_init:
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8
    mov al, 0x01
    out dx, al
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x03
    out dx, al
    mov dx, 0x3FA
    mov al, 0xC7
    out dx, al
    mov dx, 0x3FC
    mov al, 0x0B
    out dx, al
    ret

serial_putchar:
    push ax
    mov dx, 0x3FD
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    pop ax
    mov dx, 0x3F8
    out dx, al
    ret

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

boot_drive    db 0

times 510-($-$$) db 0
dw 0xAA55