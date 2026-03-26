ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : ALIGN(16)
    {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    }

    .data : ALIGN(16)
    {
        *(.data*)
    }

    .bss : ALIGN(16)
    {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}