/* REMOVED OUTPUT_FORMAT(binary) */
ENTRY(_kernel_main)

SECTIONS
{
    . = 0x1000;

    .text : {
        kernel_entry.o(.text)
        *(.text)
    }

    .data : { *(.data) }
    .bss  : { *(.bss)  }

    /DISCARD/ : {
        *(.note*)
        *(.indent)
        *(.comment)
        *(.eh_frame)
        *(.reloc)
    }
}
