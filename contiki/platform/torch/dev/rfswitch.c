
/*
 * Control the RF Switch for which IC gets the antenna
 */

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/rfswitch.h"

void rfswitch_init (void) {
	GPIO_SET_OUTPUT(RF_SWITCH_PORT_NUM, RF_SWITCH_PIN);

	// Init to CC2538
	GPIO_SET_PIN(RF_SWITCH_PORT_NUM, RF_SWITCH_PIN);
}

void rfswitch_set (rfswitch_radio_e r) {
	if (r == RFSWITCH_CC2538) {
		GPIO_SET_PIN(RF_SWITCH_PORT_NUM, RF_SWITCH_PIN);
	} else if (r == RFSWITCH_NRF51822) {
		GPIO_CLR_PIN(RF_SWITCH_PORT_NUM, RF_SWITCH_PIN);
	}
}
