/**
* \defgroup nrf51822
* @{
*/

#include "contiki.h"
#include "nrf51822.h"
#include "spi-arch.h"
#include "spi.h"
#include "dev/ssi.h"
#include "ioc.h"

/**
* \author Brad Campbell <bradjc@umich.edu>
*/

nrf51822_data_cb nrf_callback = NULL;

// Send a packet to the nRF51822 while doing the bi-directional
// thing and reading the packet it sends back
static void send_and_read (uint8_t type, uint8_t length, uint8_t* buf) {
  uint8_t response_len;
  uint8_t response_buf[256];
  int i;

  spi_set_mode(SSI_CR0_FRF_MOTOROLA, 0, 0, 8);

  SPI_CS_CLR(NRF51822_CS_N_PORT_NUM, NRF51822_CS_N_PIN);

  // There have to be at least 7.1 us of delay between CS going low and the
  // first clocked byte.
  clock_delay_usec(8);

  // Send the type of the message we are transmitting
  SPI_WRITE(type);

  // Get the length of the response packet
  SPI_WAITFOREORx();
  response_len = SPI_RXBUF;

  if (response_len == 0xFF) {
    // This means the nRF51822 has nothing to tell us, so ignore that
    response_len = 0;
  }

  // Get data from the nRF
  for (i=0; (i<response_len || i<length); i++) {

    if (i<length) {
      // we still have data to send to the nRF
      SPI_WRITE(buf[i]);
    } else {
      // we are only reading at this point
      SPI_WRITE(0);
    }

    // Make sure we have a response ready
    SPI_WAITFOREORx();

    if (i<response_len) {
      // We are reading valid data from the nRF, so store that in the buffer
      response_buf[i] = SPI_RXBUF;
    } else {
      // Just need to clear the RXBUF
      SPI_RXBUF;
    }

  }

  SPI_CS_SET(NRF51822_CS_N_PORT_NUM, NRF51822_CS_N_PIN);

  if (nrf_callback != NULL) {
    if (response_buf[0] != 0) {
      nrf_callback(response_buf[0], response_len-1, response_buf+1);
    }
  }

}

// If we get an interrupt, execute a SPI transaction to the nRF51822
// asking it what's up.
void nrf51822_interrupt(uint8_t port, uint8_t pin) {
  send_and_read(BCP_CMD_READ_IRQ, 0, NULL);
}

/**
 * \brief Initialize the nRF51822.
 */
void
nrf51822_init(nrf51822_data_cb cb)
{
  nrf_callback = cb;

  // Set the clock speed at 2 MHz
  REG(SSI0_BASE + SSI_CPSR) = 8;

  // Setup interrupt from nRF51822
  GPIO_SOFTWARE_CONTROL(NRF51822_INT_BASE, NRF51822_INT_MASK);
  GPIO_SET_INPUT(NRF51822_INT_BASE, NRF51822_INT_MASK);
  GPIO_DETECT_EDGE(NRF51822_INT_BASE, NRF51822_INT_MASK);
  GPIO_TRIGGER_SINGLE_EDGE(NRF51822_INT_BASE, NRF51822_INT_MASK);
  GPIO_DETECT_RISING(NRF51822_INT_BASE, NRF51822_INT_MASK);
  GPIO_ENABLE_INTERRUPT(NRF51822_INT_BASE, NRF51822_INT_MASK);
  ioc_set_over(NRF51822_INT_PORT_NUM, 0, IOC_OVERRIDE_DIS);
  nvic_interrupt_enable(NVIC_INT_GPIO_PORT_B);
  gpio_register_callback(nrf51822_interrupt,
                         NRF51822_INT_PORT_NUM,
                         NRF51822_INT_PIN);


  spi_cs_init(NRF51822_CS_N_PORT_NUM, NRF51822_CS_N_PIN);
  SPI_CS_SET(NRF51822_CS_N_PORT_NUM, NRF51822_CS_N_PIN);
}


void nrf51822_get_all_advertisements () {
  send_and_read(BCP_CMD_SNIFF_ADVERTISEMENTS, 0, NULL);
}

void nRF51822_set_led_state (uint8_t state) {
  uint8_t buf[1];

  buf[0] = state;
  send_and_read(BCP_CMD_UPDATE_LED_STATE, 1, buf);
}
