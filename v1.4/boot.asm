bits 32
section .multiboot
    align 4
    dd 0x1BADB002              ; Magic
    dd 0x00000007              ; Flags (Align | Mem | Video)
    dd - (0x1BADB002 + 0x00000007) ; Checksum
    dd 0, 0, 0, 0, 0
    dd 0                       ; Mode (Linear)
    dd 1024                    ; Width
    dd 768                     ; Height
    dd 32                      ; Depth

section .text
global start
extern kernel_main

start:
    cli
    mov esp, stack_top
    push ebx ; Push Multiboot Info
    push eax ; Push Magic
    call kernel_main
    hlt

section .bss
    align 16
    stack_bottom:
    resb 16384
    stack_top: