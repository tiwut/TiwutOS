bits 32

; --- MULTIBOOT HEADER ---
section .multiboot
    align 4
    dd 0x1BADB002              ; Magic
    dd 0x00000007              ; Flags (Align | MemInfo | Video)
    dd - (0x1BADB002 + 0x00000007) ; Checksum
    
    ; Address fields (Unused)
    dd 0, 0, 0, 0, 0

    ; GRAPHICS REQUEST (Standard VESA)
    dd 0                       ; Mode (0 = Linear Graphics)
    dd 800                     ; Width
    dd 600                     ; Height
    dd 32                      ; Depth (32-bit color)

; --- KERNEL ENTRY ---
section .text
global start
extern kernel_main

start:
    cli
    mov esp, stack_top
    
    ; Push Multiboot Info so C can find the Video Address
    push ebx 
    push eax
    
    call kernel_main
    
    cli
    hlt

section .bss
    align 16
    stack_bottom:
    resb 16384
    stack_top:
