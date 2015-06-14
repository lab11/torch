/*
 * Minimal cc2538 code
 *
 * Blinks an LED on the Atum board (https://github.com/lab11/atum) or SDL board
 *  (https://github.com/lab11/torch). Expects a UART bootloader to already be
 *  programmed onto the cc2538
 *
 * Branden Ghena (brghena@umich.edu) - 2015
 */

/* No includes necessary */

/* GPIO pin definitions */
#define GPIO_A_BASE  0x400D9000  // GPIO A base address
#define GPIO_B_BASE  0x400DA000  // GPIO B base address
#define GPIO_C_BASE  0x400DB000  // GPIO C base address
#define GPIO_D_BASE  0x400DC000  // GPIO D base address
#define GPIO_DIR     0x00000400  // Direction offset
#define GPIO_DATA    0x00000000  // Data offset

#define RFSWITCH_NUM  5

/* Pin definitions for various cc2538 systems */
#define ATUM_LEDS_BASE GPIO_D_BASE
#define ATUM_RED_LED   3
#define ATUM_BLUE_LED  4
#define ATUM_GREEN_LED 5

#define SDL_LEDS_BASE  GPIO_C_BASE
#define SDL_RED_LED    1
#define SDL_GREEN_LED  0
#define SDL_BIG_LED    5

#define assert_wfi() do { asm("wfi"::); } while(0)

/* Function prototypes */
void reset_handler(void);
int main(void);

/* Pointers to stack and sections */
static unsigned int stack[512]; // Allocate stack space
extern unsigned long _text;  // Linker construct indicating .text section location
extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;

/* Boot loader backdoor
 *
 * This is important for loading code through UART (via USB) instead of JTAG.
 * With a JTAG loader, this entire section is unncessary.
 */
#define FLASH_CCA_BOOTLDR_CFG_ENABLE 0xF0FFFFFF        // Enable backdoor function
#define FLASH_CCA_BOOTLDR_CFG_PORT_A_PIN_S 24          // Selected pin on pad A shift ??
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN 2   // Pin PA_2 activates the boot loader
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH 0  // A logic low level activates the boot loader
#define FLASH_CCA_BOOTLDR_CFG_ACTIVE_LEVEL 0
#define FLASH_CCA_IMAGE_VALID 0x00000000               // Indicates valid image in flash
#define FLASH_CCA_BOOTLDR_CFG ( FLASH_CCA_BOOTLDR_CFG_ENABLE \
        | FLASH_CCA_BOOTLDR_CFG_ACTIVE_LEVEL \
        | (FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN << FLASH_CCA_BOOTLDR_CFG_PORT_A_PIN_S) )

typedef struct{
    unsigned int bootldr_cfg;
    unsigned int image_valid;
    const void *app_entry_point;
    unsigned char lock[32];
} flash_cca_lock_page_t;

__attribute__ ((section(".flashcca"), used))
const flash_cca_lock_page_t __cca = {
    FLASH_CCA_BOOTLDR_CFG,         // Boot loader backdoor configuration
    FLASH_CCA_IMAGE_VALID,         // Image valid
    &_text,                        // Vector table located at the start of .text
    {   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } // Unlock all pages and debug
};

/* Vector table */
__attribute__ ((section(".vectors"), used))
void(*const vectors[])(void) = {
    (void (*)(void))((unsigned int)stack + sizeof(stack)),   /* Stack pointer */
    reset_handler,    /* Reset handler */
    0,                /* The NMI handler */
    0,                /* The hard fault handler */
    0,                /* 4 The MPU fault handler */
    0,                /* 5 The bus fault handler */
    0,                /* 6 The usage fault handler */
    0,                /* 7 Reserved */
    0,                /* 8 Reserved */
    0,                /* 9 Reserved */
    0,                /* 10 Reserved */
    0,                /* 11 SVCall handler */
    0,                /* 12 Debug monitor handler */
};

/* Reset handler
 *
 * This handler is run on reset of the microcontroller, i.e. it is the code
 *  that runs when the microcontroller starts. It needs to copy over
 *  initialization data for global variables and then enter the user code
 */
void reset_handler(void) {

    // Copy the data segment intializers from Flash to SRAM
    unsigned long* data_src = &_etext;
    unsigned long* data_dst;
    for (data_dst = &_data; data_dst < &_edata;) {
        *data_dst++ = *data_src++;
    }

    // Zero-fill the bss segment
    __asm("    ldr     r0, =_bss         \n"
          "    ldr     r1, =_ebss        \n"
          "    mov     r2, #0            \n"
          "    .thumb_func               \n"
          "zero_loop:                    \n"
          "        cmp     r0, r1        \n"
          "        it      lt            \n"
          "        strlt   r2, [r0], #4  \n"
          "        blt     zero_loop     \n"
          );

    // Call user code
    main();

    // End here if code returns
    while (1);
}

/* Main code
 *
 * This is the user's application
 */
int main(void) {

	// Sets the LED pin to be an output
    *((volatile unsigned int*)(GPIO_B_BASE | GPIO_DIR)) |= (1 << RFSWITCH_NUM);

    // switch antenna to nRF51822
    *((volatile unsigned int*)(((GPIO_B_BASE) | GPIO_DATA) + ((1 << RFSWITCH_NUM) << 2))) = 0x00;

    assert_wfi();

    return 0;
}

