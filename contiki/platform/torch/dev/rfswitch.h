#ifndef RFSWITCH_H_
#define RFSWITCH_H_

typedef enum {
	RFSWITCH_NRF51822,
	RFSWITCH_CC2538
} rfswitch_radio_e;

void rfswitch_init (void);
void rfswitch_set (rfswitch_radio_e);

#endif