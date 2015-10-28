/*

Bluetooth low energy Co-Processor

This app makes the nRF51822 into a BLE SPI slave peripheral.

See bcp.h for the list of valid commands the SPI master can issue.

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_db_discovery.h"
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "ble_advdata_parser.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "pstorage.h"
// #include "device_manager.h"
#include "app_trace.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "app_util.h"
#include "app_timer.h"

#include "simple_ble.h"
#include "eddystone.h"

#include "led.h"
#include "boards.h"

#include "bcp_spi_slave.h"
#include "ble_config.h"
#include "bcp.h"
#include "interrupt_event_queue.h"

#define PHYSWEB_URL "goo.gl/449K5X"

bool bcp_irq_advertisements = false;


#define TORCH_SHORT_UUID               0x65b8 // service UUID
#define TORCH_CHAR_LED_SHORT_UUID      0x65b9 // control the LEDs on CC2538
#define TORCH_CHAR_WHITELED_SHORT_UUID 0x65ba // control the LEDs on CC2538

// Randomly generated UUID for the torch
const ble_uuid128_t torch_uuid128 = {
    {0xef, 0x1d, 0x66, 0x4d, 0x06, 0xa5, 0x4f, 0x0a,
     0x93, 0x31, 0x23, 0x8c, 0x65, 0xb8, 0x7e, 0xb9}
};

ble_uuid_t torch_uuid = {TORCH_SHORT_UUID, BLE_UUID_TYPE_BLE};



// State for this application
static ble_app_t app;
simple_ble_app_t* simple_ble_app;


// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    .platform_id       = 0x50,              // used as 4th octect in device BLE address
    .device_id         = DEVICE_ID_DEFAULT,
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = MSEC_TO_UNITS(1000, UNIT_0_625_MS),
    .min_conn_interval = MSEC_TO_UNITS(8, UNIT_1_25_MS),
    .max_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS),
};





// Send all received advertisements to the host
void bcp_sniff_advertisements () {
    led_off(LED_0);
    bcp_irq_advertisements = true;
}

// Update the nRF's characteristic based on the setting of the CC2538 leds.
void main_set_led_state (uint8_t leds_state) {
    app.led_state = leds_state;
}



void bcp_interrupt_host () {
    nrf_gpio_pin_set(INTERRUPT_PIN);
}

void bcp_interupt_host_clear () {
    nrf_gpio_pin_clear(INTERRUPT_PIN);
}



// Function for handling the WRITE CHARACTERISTIC BLE event.
void ble_evt_write (ble_evt_t* p_ble_evt) {
    ble_gatts_evt_write_t* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == app.char_led_handles.value_handle) {
        app.led_state = p_evt_write->data[0];

        // Notify the CC2538 that the LED characteristic was written to
        interrupt_event_queue_add(BCP_RSP_LED, 1, p_evt_write->data);

    } else if (p_evt_write->handle == app.char_whiteled_handles.value_handle) {
        app.whiteled_dutycycle = p_evt_write->data[0];

        // Notify the CC2538 that the LED characteristic was written to
        interrupt_event_queue_add(BCP_RSP_WHITE_LIGHT, 1, p_evt_write->data);

    }
}



/*******************************************************************************
 *   INIT FUNCTIONS
 ******************************************************************************/


static void timers_init(void) {
    uint32_t err_code;

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
}



void services_init (void) {


    app.service_handle = simple_ble_add_service(&torch_uuid128,
                                                &torch_uuid,
                                                TORCH_SHORT_UUID);

    //add the characteristic that exposes a blob of interrupt response
    simple_ble_add_characteristic(1, 1, 0,  // read, write, notify
                                  torch_uuid.type,
                                  TORCH_CHAR_LED_SHORT_UUID,
                                  1, &app.led_state,
                                  app.service_handle,
                                  &app.char_led_handles);

    //add the characteristic that exposes a blob of interrupt response
    simple_ble_add_characteristic(1, 1, 0,  // read, write, notify
                                  torch_uuid.type,
                                  TORCH_CHAR_WHITELED_SHORT_UUID,
                                  1, &app.whiteled_dutycycle,
                                  app.service_handle,
                                  &app.char_whiteled_handles);
}




int main(void) {
    //
    // Initialization
    //

    // Setup the particular platform
    platform_init();

    // Setup BLE and services
    simple_ble_app = simple_ble_init(&ble_config);
    //simple_adv_service(&torch_uuid);

    // Setup the advertisement to use the Eddystone format.
    // We include the device name in the scan response
    ble_advdata_t srdata;
    memset(&srdata, 0, sizeof(srdata));
    srdata.name_type = BLE_ADVDATA_FULL_NAME;
    eddystone_adv(PHYSWEB_URL, &srdata);

    timers_init();

    // Make this a SPI slave to the CC2538
    spi_slave_example_init();


    while (1) {
        power_manage();
    }
}
