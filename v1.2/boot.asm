bits 32
section .multiboot
    ; --- MULTIBOOT HEADER (Must be first!) ---
    align 4
    dd 0x1BADB002              ; Magic
    dd 0x00                    ; Flags
    dd - (0x1BADB002 + 0x00)   ; Checksum

section .text
global start
extern kernel_main             ; Define entry point in C

start:
    cli                        ; Block interrupts
    mov esp, stack_top         ; Set up stack
    call kernel_main           ; Jump to C code
    hlt                        ; Stop if C returns

section .bss
    align 16
    stack_bottom:
    resb 16384                 ; 16KB Stack
    stack_top: