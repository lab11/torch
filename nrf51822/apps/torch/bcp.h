#ifndef BCP_H__
#define BCP_H__

// BCP: Bluetooth low energy Co-Processor
#define BCP_COMMAND_LEN  1


// commands
#define BCP_CMD_READ_IRQ             1 // read what caused us to interrupt the host
#define BCP_CMD_SNIFF_ADVERTISEMENTS 2 // notify host on all advertisements
#define BCP_CMD_UPDATE_LED_STATE     3 // the leds on the host changed, update characteristic

// response types
#define BCP_RSP_ADVERTISEMENT 1  // send the raw advertisement content
#define BCP_RSP_LED           2

// Send all received advertisements to the host
void bcp_sniff_advertisements ();

void bcp_interrupt_host ();
void bcp_interupt_host_clear ();


#endif