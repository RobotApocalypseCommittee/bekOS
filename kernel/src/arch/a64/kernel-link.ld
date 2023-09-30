ENTRY(_head)

SECTIONS
{
    /* Starts at this address */
    . = 0xFFFF800000000000;
/*    . = 0x40200000;*/
    __kernel_start  = .;
    .text.boot :
    {
        *(.text.boot)
    }

    .text :
    {
        *(.text .text*)
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
        *(.rodata*)
    }
    . = ALIGN(4096); /* Aligns to page size */
    __rodata_end = .;
    . = ALIGN(2 * 1024 * 1024);
    __kernel_ro_end = .;
    __kernel_rw_start = .;

    __data_start = .;
    .data :
    {
        *(.data*)
    }
    . = ALIGN(4096); /* Aligns to page size */
    __data_end = .;

    . = ALIGN(0x8);
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss*)
        *(COMMON)
    }
    __bss_end = .;
    . = ALIGN(4096);
    __stack_start = .;
    /* 8K stack space */
    . += 4096 * 2;
    __stack_top = .;

    __initial_pgtables_start = .;
    . += (4096 * 16);
    __initial_pgtables_end = .;

    . = ALIGN(2 * 1024 * 1024);
    __kernel_end = .;
    __devtree_start = .;

    __kernel_sz = __kernel_end - __kernel_start;

    .plt : {
        *(.plt) *(.plt.*) *(.iplt) *(.igot .igot.plt)
    }

    /DISCARD/ :
       {
            *(.comment)
            *(.gnu*)
            *(.note*)
            *(.eh_frame*)
            *(.data.rel.ro.*)
       }
}
