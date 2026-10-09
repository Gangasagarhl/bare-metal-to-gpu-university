; boot.asm - F1-50: a 512-byte boot sector placed at LBA 0 of the emulated NVMe disk.
; The firmware (SeaBIOS) reads it through the NVMe queues into memory at 0x7c00 and jumps
; to it. It prints one line on QEMU's debug console (I/O port 0xe9), then asks QEMU to
; exit through the isa-debug-exit device (I/O port 0xf4), so the run ends by itself.
bits 16
org 0x7c00
start:
    cli
    xor ax, ax
    mov ds, ax
    mov si, msg
.next:
    lodsb                 ; al = next byte of the message
    test al, al
    jz .done
    out 0xe9, al          ; one character to the debug console
    jmp .next
.done:
    mov al, 0x10
    out 0xf4, al          ; isa-debug-exit: QEMU stops here
.halt:
    hlt
    jmp .halt
msg: db "boot sector arrived from NVMe LBA 0 and is running", 10, 0
times 510-($-$$) db 0
dw 0xaa55                 ; boot signature checked by the firmware
