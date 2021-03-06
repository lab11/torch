
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "spi_slave.h"

#include "boards.h"
#include "led.h"

#include "bcp.h"
#include "bcp_spi_slave.h"
#include "interrupt_event_queue.h"

extern void main_set_led_state(uint8_t);

uint8_t spi_tx_buf[SPI_BUF_LEN] = {0xFF};
uint8_t spi_rx_buf[SPI_BUF_LEN] = {0};

// Keep track of whether we have put data into the SPI buffer or not.
bool buffer_full = false;

// This function is called to put data in the SPI buffer when data is added
// to the queue.
void spi_slave_notify() {
	uint32_t err_code;

	if (!buffer_full) {
		// Check if we need to repopulate the SPI buffer with the next
		// thing from the queue
		uint8_t data_len;
		uint8_t len;
		uint8_t  response_type;

		data_len = interrupt_event_queue_get(&response_type, spi_tx_buf+2);

		if (data_len > 0) {
			buffer_full = true;

			len = data_len + 1; // for the response type byte

			// Create the response SPI buffer.
			spi_tx_buf[0] = len;
			spi_tx_buf[1] = response_type;

			// Send the TX buffer to the SPI module
			err_code = spi_slave_buffers_set(spi_tx_buf,
			                                 spi_rx_buf,
			                                 SPI_BUF_LEN,
			                                 SPI_BUF_LEN);
			APP_ERROR_CHECK(err_code);

			// The interrupt will be send to the CC2538 when the data is loaded
		}
	}
}

// Callback after a SPI transaction completes (CS goes back high).
static void spi_slave_event_handle(spi_slave_evt_t event) {
	uint32_t err_code;

	// Check the event type. There are only two events, and only one is useful.
	if (event.evt_type == SPI_SLAVE_XFER_DONE) {
		// The first byte is the command byte
		switch (spi_rx_buf[0]) {

		  case BCP_CMD_READ_IRQ:
			// This message was only to read data. Success.
			break;

		  case BCP_CMD_SNIFF_ADVERTISEMENTS:
			// Instructs us to send all advertisements to the host
			// bcp_sniff_advertisements();
			break;

		  case BCP_CMD_UPDATE_LED_STATE:
		    // The LEDs on the CC2538 changed, update the characteristic
			main_set_led_state(spi_rx_buf[1]);
			break;

		  default:
			break;
		}


		{
			// Check if we need to repopulate the SPI buffer with the next
			// thing from the queue
			uint8_t data_len;
			uint8_t len;
			uint8_t response_type;

			data_len = interrupt_event_queue_get(&response_type, spi_tx_buf+2);

			if (data_len > 0) {

				len = data_len + 1; // for the response type byte

				// Create the response SPI buffer.
				spi_tx_buf[0] = len;
				spi_tx_buf[1] = response_type;

				// Send the TX buffer to the SPI module
				buffer_full = true;
				err_code = spi_slave_buffers_set(spi_tx_buf,
				                                 spi_rx_buf,
				                                 SPI_BUF_LEN,
				                                 SPI_BUF_LEN);

				APP_ERROR_CHECK(err_code);

			} else {

				// Still need to set the RX buffer as the reception
				// destination
				buffer_full = false;
				memset(spi_tx_buf, 0x0, SPI_BUF_LEN);
				memset(spi_rx_buf, 0x0, SPI_BUF_LEN);
				err_code = spi_slave_buffers_set(spi_tx_buf,
				                                 spi_rx_buf,
				                                 SPI_BUF_LEN,
				                                 SPI_BUF_LEN);
				APP_ERROR_CHECK(err_code);

				bcp_interupt_host_clear();
			}
		}

	} else if (event.evt_type == SPI_SLAVE_BUFFERS_SET_DONE) {
		nrf_gpio_pin_toggle(24);
		// If we set the buffers, we must have data we want to send
		// to the host
		if (buffer_full) {
			// Set the interrupt line high
			bcp_interrupt_host();
		}
	}

}


uint32_t spi_slave_example_init(void)
{
	uint32_t           err_code;
	spi_slave_config_t spi_slave_config;

	// This callback fires after the master has de-asserted chip select
	err_code = spi_slave_evt_handler_register(spi_slave_event_handle);
	APP_ERROR_CHECK(err_code);

	// Setup the pins from the board's .h file
	spi_slave_config.pin_miso         = SPIS_MISO_PIN;
	spi_slave_config.pin_mosi         = SPIS_MOSI_PIN;
	spi_slave_config.pin_sck          = SPIS_SCK_PIN;
	spi_slave_config.pin_csn          = SPIS_CSN_PIN;
	spi_slave_config.mode             = SPI_MODE_0;
	spi_slave_config.bit_order        = SPIM_MSB_FIRST;
	spi_slave_config.def_tx_character = 0x00;
	spi_slave_config.orc_tx_character = 0x55;

	err_code = spi_slave_init(&spi_slave_config);
	APP_ERROR_CHECK(err_code);

	// Set buffers so we can receive
	err_code = spi_slave_buffers_set(spi_tx_buf,
									 spi_rx_buf,
									 SPI_BUF_LEN,
									 SPI_BUF_LEN);
	APP_ERROR_CHECK(err_code);

	return NRF_SUCCESS;
}

