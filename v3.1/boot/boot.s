.set ALIGN,    1<<0             
.set MEMINFO,  1<<1             
.set VIDEO,    1<<2
.set FLAGS,    ALIGN | MEMINFO | VIDEO
.set MAGIC,    0x1BADB002       
.set CHECKSUM, -(MAGIC + FLAGS) 

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0, 0, 0, 0, 0
.long 0 # 0 = linear graphics
.long 800
.long 600
.long 32

.section .bss
.align 4096
pml4_table: .skip 4096
pdpt_table: .skip 4096
pd_table:   .skip 16384 # 4 Page Directories for 4GB
.align 16
stack_bottom: .skip 16384
stack_top:

.section .rodata
.align 8
gdt64:
    .quad 0 # null
    .quad 0x00209A0000000000 # 64-bit code
    .quad 0x0000920000000000 # 64-bit data
.align 2
gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global start
start:
    mov $stack_top, %esp
    mov %ebx, %edi # Save Multiboot Info to EDI for C++

    # Map first 4GB
    mov $pdpt_table, %eax
    or $0b11, %eax
    mov %eax, pml4_table

    mov $0, %ecx
map_pdpt:
    mov $pd_table, %eax
    mov %ecx, %ebx
    shl $12, %ebx
    add %ebx, %eax
    or $0b11, %eax
    mov %eax, pdpt_table(,%ecx,8)
    inc %ecx
    cmp $4, %ecx
    jne map_pdpt

    mov $0, %ecx
map_pd:
    mov $0x200000, %eax
    mul %ecx
    or $0b10000011, %eax # Huge page 2MB + Present + Write
    mov %eax, pd_table(,%ecx,8)
    inc %ecx
    cmp $2048, %ecx
    jne map_pd

    # Load PML4
    mov $pml4_table, %eax
    mov %eax, %cr3

    # Enable PAE
    mov %cr4, %eax
    or $1<<5, %eax
    mov %eax, %cr4

    # Enable Long Mode
    mov $0xC0000080, %ecx
    rdmsr
    or $1<<8, %eax
    wrmsr

    # Enable Paging
    mov %cr0, %eax
    or $1<<31, %eax
    mov %eax, %cr0

    # Load GDT
    lgdt gdt64_pointer

    # Jump to 64-bit
    ljmp $8, $long_mode_start

.code64
long_mode_start:
    mov $16, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Call TiwutOS
    call cpp_main

halt:
    cli
    hlt
    jmp halt
