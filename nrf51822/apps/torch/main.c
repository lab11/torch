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
#include "simple_adv.h"

#include "led.h"
#include "boards.h"

#include "bcp_spi_slave.h"
#include "ble_config.h"
#include "bcp.h"
#include "interrupt_event_queue.h"


bool bcp_irq_advertisements = false;


#define TORCH_SHORT_UUID             0x65b8 // service UUID
#define TORCH_CHAR_LED_SHORT_UUID    0x65b9 // control the LEDs on CC2538

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





/**@brief Function for handling the WRITE event.
 */
static void on_write (ble_evt_t* p_ble_evt)
{
    ble_gatts_evt_write_t* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == app.char_led_handles.value_handle) {
        app.led_state = p_evt_write->data[0];

        // Notify the CC2538 that the LED characteristic was written to
        interrupt_event_queue_add(BCP_RSP_LED, 1, p_evt_write->data);
    }
}



/*******************************************************************************
 *   INIT FUNCTIONS
 ******************************************************************************/


static void timers_init(void) {
    uint32_t err_code;

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);

    // err_code = app_timer_create(&characteristic_timer,
    //                             APP_TIMER_MODE_REPEATED,
    //                             timer_handler);
    // APP_ERROR_CHECK(err_code);

    // // Start timer to update characteristic
    // err_code = app_timer_start(characteristic_timer, UPDATE_RATE, NULL);
    // APP_ERROR_CHECK(err_code);
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





    // uint32_t err_code;

    // // Setup our long UUID so that nRF recognizes it. This is done by
    // // storing the full UUID and essentially using `torch_uuid`
    // // as a handle.
    // torch_uuid.uuid = TORCH_SHORT_UUID;
    // err_code = sd_ble_uuid_vs_add(&torch_uuid128, &(torch_uuid.type));
    // APP_ERROR_CHECK(err_code);
    // app.uuid_type = torch_uuid.type;

    // // Add the custom service to the system
    // err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
    //                                     &torch_uuid,
    //                                     &(app.service_handle));
    // APP_ERROR_CHECK(err_code);

    // // Add the LED characteristic to the service
    // {
    //     ble_gatts_char_md_t char_md;
    //     ble_gatts_attr_t    attr_char_value;
    //     ble_uuid_t          char_uuid;
    //     ble_gatts_attr_md_t attr_md;

    //     // Init value
    //     app.led_state = 0;

    //     memset(&char_md, 0, sizeof(char_md));

    //     // This characteristic is a read & write
    //     char_md.char_props.read          = 1;
    //     char_md.char_props.write         = 1;
    //     char_md.p_char_user_desc         = NULL;
    //     char_md.p_char_pf                = NULL;
    //     char_md.p_user_desc_md           = NULL;
    //     char_md.p_cccd_md                = NULL;
    //     char_md.p_sccd_md                = NULL;

    //     char_uuid.type = app.uuid_type;
    //     char_uuid.uuid = TORCH_CHAR_LED_SHORT_UUID;

    //     memset(&attr_md, 0, sizeof(attr_md));

    //     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    //     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    //     attr_md.vloc    = BLE_GATTS_VLOC_USER;
    //     attr_md.rd_auth = 0;
    //     attr_md.wr_auth = 0;
    //     attr_md.vlen    = 1;

    //     memset(&attr_char_value, 0, sizeof(attr_char_value));

    //     attr_char_value.p_uuid    = &char_uuid;
    //     attr_char_value.p_attr_md = &attr_md;
    //     attr_char_value.init_len  = 1;
    //     attr_char_value.init_offs = 0;
    //     attr_char_value.max_len   = 1;
    //     attr_char_value.p_value   = (uint8_t*) &app.led_state;

    //     err_code = sd_ble_gatts_characteristic_add(app.service_handle,
    //                                                &char_md,
    //                                                &attr_char_value,
    //                                                &app.char_led_handles);
    //     APP_ERROR_CHECK(err_code);
    // }

}




int main(void) {
    //
    // Initialization
    //

    // Setup the particular platform
    platform_init();

    // Setup BLE and services
    simple_ble_app = simple_ble_init(&ble_config);
    simple_adv_service(&torch_uuid);

    timers_init();

    // Make this a SPI slave to the CC2538
    spi_slave_example_init();


    while (1) {
        power_manage();
    }
}
