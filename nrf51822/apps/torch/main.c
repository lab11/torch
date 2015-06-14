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

ble_uuid_t torch_uuid;

// Security requirements for this application.
static ble_gap_sec_params_t m_sec_params = {
    SEC_PARAM_TIMEOUT,
    SEC_PARAM_BOND,
    SEC_PARAM_MITM,
    SEC_PARAM_IO_CAPABILITIES,
    SEC_PARAM_OOB,
    SEC_PARAM_MIN_KEY_SIZE,
    SEC_PARAM_MAX_KEY_SIZE,
};

// State for this application
static ble_app_t app;

static ble_gap_adv_params_t m_adv_params;

// static app_timer_id_t  characteristic_timer;





/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // APPL_LOG("[APPL]: ASSERT: %s, %d, error 0x%08x\r\n", p_file_name, line_num, error_code);
    // nrf_gpio_pin_set(ASSERT_LED_PIN_NO);

    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    NVIC_SystemReset();
}


/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}



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


static void advertising_start(void) {

    //start advertising
    uint32_t             err_code;
    err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);
}

static void advertising_stop(void) {
    //start advertising
    uint32_t             err_code;
    err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
}


//service error callback
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
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


//connection parameters event handler callback
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(app.conn_handle,
                                         BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

//connection parameters error callback
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}





/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt) {
    uint32_t                         err_code;
    static ble_gap_evt_auth_status_t m_auth_status;
    ble_gap_enc_info_t *             p_enc_info;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            app.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            //advertising_stop();
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            app.conn_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start();
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_ble_evt);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(app.conn_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &m_sec_params);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(app.conn_handle, NULL, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            p_enc_info = &m_auth_status.periph_keys.enc_info;
            if (p_enc_info->div ==
                            p_ble_evt->evt.gap_evt.params.sec_info_request.div) {
                err_code = sd_ble_gap_sec_info_reply(app.conn_handle,
                                                            p_enc_info, NULL);
                APP_ERROR_CHECK(err_code);
            } else {
                // No keys found for this device
                err_code = sd_ble_gap_sec_info_reply(app.conn_handle, NULL, NULL);
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT) {
                err_code = sd_power_system_off();
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            break;
    }
}

// /**@brief Function for handling the Application's system events.
//  *
//  * @param[in]   sys_evt   system event.
//  */
// static void on_sys_evt(uint32_t sys_evt)
// {
//     switch(sys_evt)
//     {
//         case NRF_EVT_FLASH_OPERATION_SUCCESS:
//         case NRF_EVT_FLASH_OPERATION_ERROR:
//             if (m_memory_access_in_progress)
//             {
//                 m_memory_access_in_progress = false;
//                 scan_start();
//             }
//             break;
//         default:
//             // No implementation needed.
//             break;
//     }
// }


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *  been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
   // dm_ble_evt_handler(p_ble_evt);
   // ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
   // ble_hrs_c_on_ble_evt(&m_ble_hrs_c, p_ble_evt);
   // ble_bas_c_on_ble_evt(&m_ble_bas_c, p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    // pstorage_sys_event_handler(sys_evt);
    // on_sys_evt(sys_evt);
}


// static void timer_handler (void* p_context) {
//     cstm.num_value++;
// }


/*******************************************************************************
 *   INIT FUNCTIONS
 ******************************************************************************/

// Initialize connection parameters
static void conn_params_init(void) {
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

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

// initialize advertising
static void advertising_init(void) {
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;
    uint8_t       flags =  BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    // Advertise our custom service
    ble_uuid_t adv_uuids[] = {torch_uuid};

    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));
    memset(&srdata, 0, sizeof(srdata));

    advdata.name_type               = BLE_ADVDATA_NO_NAME;
    advdata.include_appearance      = true;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;

    // Put the name in the SCAN RESPONSE data
    srdata.name_type                = BLE_ADVDATA_FULL_NAME;

    err_code = ble_advdata_set(&advdata, &srdata);
    APP_ERROR_CHECK(err_code);

    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.type               = BLE_GAP_ADV_TYPE_ADV_IND;
    m_adv_params.p_peer_addr        = NULL;
    m_adv_params.fp                 = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval           = APP_ADV_INTERVAL;
    m_adv_params.timeout            = APP_ADV_TIMEOUT_IN_SECONDS;
}

//init services
static void services_init (void)
{
    uint32_t err_code;

    // Setup our long UUID so that nRF recognizes it. This is done by
    // storing the full UUID and essentially using `torch_uuid`
    // as a handle.
    torch_uuid.uuid = TORCH_SHORT_UUID;
    err_code = sd_ble_uuid_vs_add(&torch_uuid128, &(torch_uuid.type));
    APP_ERROR_CHECK(err_code);
    app.uuid_type = torch_uuid.type;

    // Add the custom service to the system
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &torch_uuid,
                                        &(app.service_handle));
    APP_ERROR_CHECK(err_code);

    // Add the LED characteristic to the service
    {
        ble_gatts_char_md_t char_md;
        ble_gatts_attr_t    attr_char_value;
        ble_uuid_t          char_uuid;
        ble_gatts_attr_md_t attr_md;

        // Init value
        app.led_state = 0;

        memset(&char_md, 0, sizeof(char_md));

        // This characteristic is a read & write
        char_md.char_props.read          = 1;
        char_md.char_props.write         = 1;
        char_md.p_char_user_desc         = NULL;
        char_md.p_char_pf                = NULL;
        char_md.p_user_desc_md           = NULL;
        char_md.p_cccd_md                = NULL;
        char_md.p_sccd_md                = NULL;

        char_uuid.type = app.uuid_type;
        char_uuid.uuid = TORCH_CHAR_LED_SHORT_UUID;

        memset(&attr_md, 0, sizeof(attr_md));

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

        attr_md.vloc    = BLE_GATTS_VLOC_USER;
        attr_md.rd_auth = 0;
        attr_md.wr_auth = 0;
        attr_md.vlen    = 1;

        memset(&attr_char_value, 0, sizeof(attr_char_value));

        attr_char_value.p_uuid    = &char_uuid;
        attr_char_value.p_attr_md = &attr_md;
        attr_char_value.init_len  = 1;
        attr_char_value.init_offs = 0;
        attr_char_value.max_len   = 1;
        attr_char_value.p_value   = (uint8_t*) &app.led_state;

        err_code = sd_ble_gatts_characteristic_add(app.service_handle,
                                                   &char_md,
                                                   &attr_char_value,
                                                   &app.char_led_handles);
        APP_ERROR_CHECK(err_code);
    }

}


// gap name/appearance/connection parameters
static void gap_params_init (void) {
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    // Full strength signal
    sd_ble_gap_tx_power_set(4);

    // Let anyone connect and set the name given the platform
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    // Not sure what this is useful for, but why not set it
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_COMPUTER);
    APP_ERROR_CHECK(err_code);

    // Specify parameters for a connection
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init (void) {
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(
        NRF_CLOCK_LFCLKSRC_RC_250_PPM_8000MS_CALIBRATION, false);

    // Enable BLE stack
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed =
                                            IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    //Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Set the MAC address of the device
    {
        ble_gap_addr_t gap_addr;

        // Get the current original address
        sd_ble_gap_address_get(&gap_addr);

        // Set the new BLE address with the Michigan OUI
        gap_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
        memcpy(gap_addr.addr+2, MAC_ADDR+2, sizeof(gap_addr.addr)-2);
        err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &gap_addr);
        APP_ERROR_CHECK(err_code);
    }
}

/** @brief Function for the Power manager.
 */
static void power_manage (void) {
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


int main(void) {
    //
    // Initialization
    //

    // Setup the particular platform
    platform_init();

    // Setup BLE and services
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    timers_init();
    conn_params_init();

    // Make this a SPI slave to the CC2538
    spi_slave_example_init();

    // Advertise that we are a TORCH board
    advertising_start();

    while (1) {
        power_manage();
    }
}
