#include "torch.h"

#include "led.h"


void platform_init () {

	// Setup the interrupt pin to the CC2538
	nrf_gpio_cfg_output(INTERRUPT_PIN);
    nrf_gpio_pin_clear(INTERRUPT_PIN);

}

