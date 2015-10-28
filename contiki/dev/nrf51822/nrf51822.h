#ifndef NRF51822_H_
#define NRF51822_H_

// commands
#define BCP_CMD_READ_IRQ             1 // read what caused us to interrupt the host
#define BCP_CMD_SNIFF_ADVERTISEMENTS 2 // notify host on all advertisements
#define BCP_CMD_UPDATE_LED_STATE     3 // the leds on the host changed, update characteristic


typedef void (*nrf51822_data_cb)(uint8_t type, uint8_t len, uint8_t* buf);

void nrf51822_interrupt(uint8_t port, uint8_t pin);
void nrf51822_init(nrf51822_data_cb cb);

//
// NOTE: not all the functions may necessarily be supported by the
//       client nRF51822
//

// Tell the nRF51822 to send us all advertisements
void nrf51822_get_all_advertisements ();

// Tell the nRF the current state of the RGB leds on the board
void nRF51822_set_led_state (uint8_t state);

// response types
#define BCP_RSP_ADVERTISEMENT 1  // send the raw advertisement content
#define BCP_RSP_LED           2
#define BCP_RSP_WHITE_LIGHT   3  // configure the main bright white led

#endif