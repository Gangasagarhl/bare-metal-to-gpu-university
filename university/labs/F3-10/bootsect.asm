; bootsect.asm - milestone A5 (optional, history): a legacy BIOS boot sector and a second stage.
; Assemble: nasm -f bin bootsect.asm -o disk.img   (two 512-byte sectors)
; The BIOS (SeaBIOS in QEMU) loads sector 0 at 0000:7C00 and jumps to it in 16-bit real mode.
; Stage 1 reads sector 1 with the BIOS extended disk service (INT 13h, AH=42h) to 0000:7E00.
; Stage 2 asks the BIOS for the memory map (INT 15h, EAX=E820h) and prints every entry.
; Text goes straight to the first serial port (I/O port 0x3F8), because what the BIOS draws on
; the VGA screen is not part of a serial log. Service numbers and register conventions were
; written from memory: check them in the documents named in F3-10 (unverified box there).
        bits 16
        org 0x7c00

COM1    equ 0x3f8
stage1:
        cli
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, 0x7c00              ; stack grows down, below our own code
        sti
        mov [boot_drive], dl        ; the BIOS passes the boot drive number in DL
        call serial_init
        mov si, msg_hello
        call puts
        mov al, [boot_drive]
        call hex8
        call newline

        mov ah, 0x41                ; are the extended disk services present?
        mov bx, 0x55aa
        mov dl, [boot_drive]
        int 0x13
        jc disk_error
        mov ah, 0x42                ; extended read, using the disk address packet below
        mov dl, [boot_drive]
        mov si, dap
        int 0x13
        jc disk_error
        mov si, msg_loaded
        call puts
        jmp 0x0000:stage2

disk_error:
        mov si, msg_disk
        call puts
.halt:  hlt
        jmp .halt

serial_init:                        ; 16550 UART: 8 data bits, no parity, 1 stop bit
        mov dx, COM1 + 1
        xor al, al
        out dx, al                  ; no UART interrupts
        mov dx, COM1 + 3
        mov al, 0x80
        out dx, al                  ; divisor latch access on
        mov dx, COM1
        mov al, 1
        out dx, al                  ; divisor low byte = 1
        mov dx, COM1 + 1
        xor al, al
        out dx, al                  ; divisor high byte = 0
        mov dx, COM1 + 3
        mov al, 0x03
        out dx, al                  ; 8N1, divisor latch off
        ret

putc:                               ; AL = character
        push dx
        push ax
        mov dx, COM1 + 5
.wait:  in al, dx
        test al, 0x20               ; transmit holding register empty?
        jz .wait
        pop ax
        mov dx, COM1
        out dx, al
        pop dx
        ret

puts:                               ; DS:SI = zero-terminated string
        lodsb
        test al, al
        jz .done
        call putc
        jmp puts
.done:  ret

newline:
        mov al, 13
        call putc
        mov al, 10
        jmp putc

hex8:                               ; prints AL as two hex digits
        push ax
        shr al, 4
        call .digit
        pop ax
        and al, 0x0f
.digit: and al, 0x0f
        add al, '0'
        cmp al, '9'
        jbe .out
        add al, 'a' - '0' - 10
.out:   jmp putc

hex32:                              ; prints EAX as eight hex digits
        push eax
        rol eax, 8
        call hex8
        rol eax, 8
        call hex8
        rol eax, 8
        call hex8
        rol eax, 8
        call hex8
        pop eax
        ret

boot_drive: db 0
dap:    db 16, 0                    ; disk address packet: size, reserved
        dw 1                        ; sectors to read
        dw 0x7e00, 0x0000           ; buffer offset, segment
        dq 1                        ; first LBA to read (sector 1, the second sector)
msg_hello:  db "A5 stage 1: running at 0000:7c00 in real mode, boot drive 0x", 0
msg_loaded: db "A5 stage 1: stage 2 read with INT 13h AH=42h", 13, 10, 0
msg_disk:   db "A5 stage 1: disk read failed", 13, 10, 0

        times 510 - ($ - $$) db 0
        dw 0xaa55                   ; boot signature: bytes 0x55 0xAA at offsets 510 and 511

; ------------------------------------------------------------------ sector 1: stage 2
stage2:
        mov si, msg_e820
        call puts
        xor ebx, ebx                ; continuation value: 0 = start
        xor bp, bp                  ; entry counter
.next:  mov di, entry
        mov eax, 0xe820
        mov edx, 0x534d4150         ; 'SMAP'
        mov ecx, 24
        int 0x15
        jc .end                     ; carry set: no more entries (or not supported)
        cmp eax, 0x534d4150
        jne .end
        inc bp
        mov si, msg_base
        call puts
        mov eax, [entry + 4]        ; base, high half then low half
        call hex32
        mov eax, [entry]
        call hex32
        mov si, msg_len
        call puts
        mov eax, [entry + 12]       ; length
        call hex32
        mov eax, [entry + 8]
        call hex32
        mov si, msg_type
        call puts
        mov al, [entry + 16]        ; type (1 = usable RAM)
        call hex8
        call newline
        test ebx, ebx               ; 0 = that was the last entry
        jnz .next
.end:   mov si, msg_count
        call puts
        mov ax, bp
        call hex8
        call newline
        mov dx, 0xf4                ; QEMU isa-debug-exit: QEMU exits with status (0x10 << 1) | 1 = 33
        mov al, 0x10
        out dx, al
.halt:  hlt
        jmp .halt

msg_e820:  db "A5 stage 2: BIOS memory map (INT 15h, EAX=E820h)", 13, 10, 0
msg_base:  db "  base 0x", 0
msg_len:   db "  length 0x", 0
msg_type:  db "  type ", 0
msg_count: db "A5 stage 2: entries 0x", 0
entry:     times 24 db 0

        times 1024 - ($ - $$) db 0
