ENTRY(_start)

SECTIONS
{
    /* Starts at this address */
    . = 0xFFFF000000080000;
    __start  = .;
    .text.boot :
    {
        *(.text.boot)
    }

    .text :
    {
        *(.text)
    }

    . = ALIGN(4096); /* Aligns to page size */
    __vector_table_start = .;
    .text.vec :
    {
        *(.text.vec)
    }
    . = ALIGN(4096);
    __vector_table_end = .;

    __rodata_start = .;
    .rodata :
    {
        *(.rodata)
    }
    . = ALIGN(4096); /* Aligns to page size */
    __rodata_end = .;

    __data_start = .;
    .data :
    {
        *(.data)
    }
    . = ALIGN(4096); /* Aligns to page size */
    __data_end = .;

    . = ALIGN(0x8);
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
    }
    __bss_end = .;

    . = ALIGN(4096);
    __page_table_start = .;
    .data.pgt :
    {
    . += (6 * (1 << 12));
    }
    __page_table_end = .;
    __end = .;

    /DISCARD/ :
       {
            *(.comment)
            *(.gnu*)
            *(.note*)
            *(.eh_frame*)
       }
}
