; boot.asm - F3-48: an El Torito "no emulation" boot image. The BIOS copies it to
; 0x0000:0x7C00 and jumps to it in 16-bit real mode with DL = the BIOS drive number.
; It prints one line on the first serial port, then exits QEMU through isa-debug-exit.
bits 16
org 0x7C00
start:
    cli
    jmp 0x0000:norm             ; make CS = 0 whatever segment the BIOS jumped from
norm:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [drive], dl             ; remember the drive number the BIOS gave us
    mov si, msg
    call puts
    mov al, [drive]             ; print it as two hex digits
    shr al, 4
    call hexdigit
    mov al, [drive]
    and al, 0x0F
    call hexdigit
    mov si, crlf
    call puts
    mov al, 0x10
    out 0xF4, al                ; isa-debug-exit: QEMU exits with status (0x10 << 1) | 1 = 33
.halt:
    hlt
    jmp .halt

hexdigit:                       ; AL = 0..15
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 'A' - '9' - 1
.out:
    mov bl, al
    jmp putc

puts:                           ; DS:SI = zero-terminated string
    mov bl, [si]
    inc si
    test bl, bl
    jz .done
    call putc
    jmp puts
.done:
    ret

putc:                           ; BL = character; wait until COM1 can take it
    mov dx, 0x3FD               ; line status register
.wait:
    in al, dx
    test al, 0x20               ; transmitter holding register empty
    jz .wait
    mov dx, 0x3F8
    mov al, bl
    out dx, al
    ret

msg:   db "OS401: El Torito no-emulation boot image running; BIOS drive 0x", 0
crlf:  db 13, 10, 0
drive: db 0
times 2048 - ($ - $$) db 0      ; one 2048-byte CD sector
