ENTRY(start)

SECTIONS {
    . = 1M; /* Load kernel at 1MB */

    .text : {
        /* Force multiboot header to be first */
        *(.multiboot)
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}