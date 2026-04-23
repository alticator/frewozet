ENTRY(_start)

KERNEL_VIRT_BASE  = 0XC0000000;
KERNEL_PHYS_START = 0X00100000;
KERNEL_VIRT_START = KERNEL_PHYS_START + KERNEL_VIRT_BASE;

SECTIONS
{
    . = KERNEL_VIRT_START;

    .text : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text.start)
        *(.text*)
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata*)
    }

    .data : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data*)
    }

    .bss : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss*)
    }

    _kernel_start = ADDR(.text);
    _kernel_end = .;

    _kernel_phys_start = LOADADDR(.text);
    _kernel_phys_end = LOADADDR(.bss) + SIZEOF(.bss);

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}