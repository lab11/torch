#ifndef NRF51822_H_
#define NRF51822_H_

typedef void (*nrf51822_data_cb)(uint8_t type, uint8_t len, uint8_t* buf);

void nrf51822_interrupt(uint8_t port, uint8_t pin);
void nrf51822_init(nrf51822_data_cb cb);
void nrf51822_get_all_advertisements ();

// response types
#define BCP_RSP_ADVERTISEMENT 1  // send the raw advertisement content
#define BCP_RSP_LED           2

#endif