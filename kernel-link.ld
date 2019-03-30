ENTRY(_start)

SECTIONS
{
    /* Starts at this address */
    . = 0x80000;
    __start  = .;
    __virt_start = . + 0xffff000000000000;
    .text.boot : AT(0x80000)
        {
            *(.text.boot)
        }
    __boot_end = .;
    . += 0xffff000000000000;
    __text_start = .;
    .text : AT(__boot_end)
    {
        *(.text)
    }
    . = ALIGN(4096); /* Aligns to page size */
    __text_end = .;

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
    __bss_size = SIZEOF(.bss);
    . = ALIGN(4096);
    __page_table_start = .;
    .data.pgt :
    {
    . += (6 * (1 << 12));
    }
    __page_table_end = .;
    __hard_bss_start = LOADADDR(.bss);
    __hard_page_table_start = LOADADDR(.data.pgt);
    __end = .;
}
