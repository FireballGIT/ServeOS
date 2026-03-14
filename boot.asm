[org 0x7c00]
KERNEL_OFFSET equ 0x1000

start:
    mov [BOOT_DRIVE], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Load Kernel from Disk (Real Mode)
    mov bx, KERNEL_OFFSET
    mov dh, 10              ; Read 10 sectors
    mov dl, [BOOT_DRIVE]
    mov ah, 0x02
    mov al, dh
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    int 0x13

    ; Switch to Protected Mode
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load GDT
    mov eax, cr0
    or eax, 0x1             ; Set PE bit in CR0
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; Far jump to flush pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG        ; Update segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000        ; Update stack for 32-bit
    mov esp, ebp

    call KERNEL_OFFSET      ; Jump to the C Kernel
    jmp $

; GDT Data
gdt_start:
    dq 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
BOOT_DRIVE db 0

times 510-($-$$) db 0
dw 0xaa55
