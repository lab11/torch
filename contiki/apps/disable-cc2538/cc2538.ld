MEMORY {
    FLASH (rx) : ORIGIN = 0x00200000, LENGTH = (0x00080000 - 44)
    FLASH_CCA (RX) : ORIGIN = (0x00200000 + (0x00080000 - 44)), LENGTH = 44
    SRAM (RWX) : ORIGIN = 0x20000000, LENGTH = 0x00008000
}

SECTIONS {
/* Text section, Code! Also vector table */
    .text :
    {
        _text = .;
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH =0xFF

/* Data section, static initialized variables
 *  Note: This is placed in Flash after the text section, but needs to be
 *  moved to SRAM at runtime
 */
    .data :
    {
        _data = .;
        *(.data*)
        _edata = .;
    } > SRAM AT > FLASH

/* BSS section, static uninitialized variables */
    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

/* Flash Customer Configuration Area, bootloader */
    .flashcca :
    {
        KEEP(*(.flashcca))
    } > FLASH_CCA
}
