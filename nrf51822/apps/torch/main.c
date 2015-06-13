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
#include "device_manager.h"
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


// #define LED_GOT_ADV_PACKET               LED_0                                          /**< Is on when application has asserted. */

// #define INTERRUPT_PIN                    7
// #define ADV_PIN                    4

// #define APPL_LOG                         app_trace_log                                  /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */

// #define SEC_PARAM_BOND             1                                  /**< Perform bonding. */
// #define SEC_PARAM_MITM             1                                  /**< Man In The Middle protection not required. */
// #define SEC_PARAM_IO_CAPABILITIES  BLE_GAP_IO_CAPS_NONE               /**< No I/O capabilities. */
// #define SEC_PARAM_OOB              0                                  /**< Out Of Band data not available. */
// #define SEC_PARAM_MIN_KEY_SIZE     7                                  /**< Minimum encryption key size. */
// #define SEC_PARAM_MAX_KEY_SIZE     16                                 /**< Maximum encryption key size. */

// #define SCAN_INTERVAL              0x00A0                             /**< Determines scan interval in units of 0.625 millisecond. */
// #define SCAN_WINDOW                0x0050                             /**< Determines scan window in units of 0.625 millisecond. */

// #define MIN_CONNECTION_INTERVAL    MSEC_TO_UNITS(7.5, UNIT_1_25_MS)   /**< Determines maximum connection interval in millisecond. */
// #define MAX_CONNECTION_INTERVAL    MSEC_TO_UNITS(30, UNIT_1_25_MS)    /**< Determines maximum connection interval in millisecond. */
// #define SLAVE_LATENCY              0                                  /**< Determines slave latency in counts of connection events. */
// #define SUPERVISION_TIMEOUT        MSEC_TO_UNITS(4000, UNIT_10_MS)    /**< Determines supervision time-out in units of 10 millisecond. */

// #define TARGET_UUID                0x180D                             /**< Target device name that application is looking for. */
// #define MAX_PEER_COUNT             DEVICE_MANAGER_MAX_CONNECTIONS     /**< Maximum number of peer's application intends to manage. */
// #define UUID16_SIZE                2                                  /**< Size of 16 bit UUID */

// /**@breif Macro to unpack 16bit unsigned UUID from octet stream. */
// #define UUID16_EXTRACT(DST,SRC)                                                                  \
//         do                                                                                       \
//         {                                                                                        \
//             (*(DST)) = (SRC)[1];                                                                 \
//             (*(DST)) <<= 8;                                                                      \
//             (*(DST)) |= (SRC)[0];                                                                \
//         } while(0)

// /**@brief Variable length data encapsulation in terms of length and pointer to data */
// typedef struct
// {
//     uint8_t     * p_data;                                         /**< Pointer to data. */
//     uint16_t      data_len;                                       /**< Length of data. */
// }data_t;

// typedef enum
// {
//     BLE_NO_SCAN,                                                  /**< No advertising running. */
//     BLE_WHITELIST_SCAN,                                           /*< Advertising with whitelist.
//     BLE_FAST_SCAN,                                                /**< Fast advertising running. */
// } ble_advertising_mode_t;

// static ble_db_discovery_t           m_ble_db_discovery;                  /**< Structure used to identify the DB Discovery module. */
// static ble_hrs_c_t                  m_ble_hrs_c;                         /**< Structure used to identify the heart rate client module. */
// static ble_bas_c_t                  m_ble_bas_c;                         /**< Structure used to identify the Battery Service client module. */
// static ble_gap_scan_params_t        m_scan_param;                        /**< Scan parameters requested for scanning and connection. */
// static dm_application_instance_t    m_dm_app_id;                         /**< Application identifier. */
// static dm_handle_t                  m_dm_device_handle;                  /**< Device Identifier identifier. */
// static uint8_t                      m_peer_count = 0;                    /**< Number of peer's connected. */
// static uint8_t                      m_scan_mode;                         /**< Scan mode used by application. */

// static bool                         m_memory_access_in_progress = false; /**< Flag to keep track of ongoing operations on persistent memory. */

// /**
//  * @brief Connection parameters requested for connection.
//  */
// static const ble_gap_conn_params_t m_connection_param =
// {
//     (uint16_t)MIN_CONNECTION_INTERVAL,   // Minimum connection
//     (uint16_t)MAX_CONNECTION_INTERVAL,   // Maximum connection
//     0,                                   // Slave latency
//     (uint16_t)SUPERVISION_TIMEOUT        // Supervision time-out
// };

// static void scan_start(void);

// #define APPL_LOG                        app_trace_log             /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */

// WARNING: The following macro MUST be un-defined (by commenting out the definition) if the user
// does not have a nRF6350 Display unit. If this is not done, the application will not work.
//#define APPL_LCD_PRINT_ENABLE                                     /**< In case you do not have a functional display unit, disable this flag and observe trace on UART. */

// #ifdef APPL_LCD_PRINT_ENABLE

// #define APPL_LCD_CLEAR                  nrf6350_lcd_clear         /**< Macro to clear the LCD display.*/
// #define APPL_LCD_WRITE                  nrf6350_lcd_write_string  /**< Macro to write a string to the LCD display.*/

// #else // APPL_LCD_PRINT_ENABLE

// #define APPL_LCD_WRITE(...)             true                      /**< Macro to clear the LCD display defined to do nothing when @ref APPL_LCD_PRINT_ENABLE is not defined.*/
// #define APPL_LCD_CLEAR(...)             true                      /**< Macro to write a string to the LCD display defined to do nothing when @ref APPL_LCD_PRINT_ENABLE is not defined.*/

// #endif // APPL_LCD_PRINT_ENABLE




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
        err_code = sd_ble_gap_disconnect(app.conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
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
static void services_init(void)
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
static void gap_params_init(void) {
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    sd_ble_gap_tx_power_set(4);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
    APP_ERROR_CHECK(err_code);

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
static void ble_stack_init(void) {
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_8000MS_CALIBRATION,
                                                                        false);

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

    // Set the address
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


// /**@brief Function for initializing the Device Manager.
//  *
//  * @details Device manager is initialized here.
//  */
// static void device_manager_init(void)
// {
//     dm_application_param_t param;
//     dm_init_param_t        init_param;

//     uint32_t              err_code;

//     err_code = pstorage_init();
//     APP_ERROR_CHECK(err_code);

//     // Clear all bonded devices if user requests to.
//     init_param.clear_persistent_data = false;
//         //((nrf_gpio_pin_read(BOND_DELETE_ALL_BUTTON_ID) == 0)? true: false);

//     err_code = dm_init(&init_param);
//     APP_ERROR_CHECK(err_code);

//     memset(&param.sec_param, 0, sizeof (ble_gap_sec_params_t));

//     // Event handler to be registered with the module.
//     param.evt_handler            = device_manager_event_handler;

//     // Service or protocol context for device manager to load, store and apply on behalf of application.
//     // Here set to client as application is a GATT client.
//     param.service_type           = DM_PROTOCOL_CNTXT_GATT_CLI_ID;

//     // Secuirty parameters to be used for security procedures.
//     param.sec_param.bond         = SEC_PARAM_BOND;
//     param.sec_param.mitm         = SEC_PARAM_MITM;
//     param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
//     param.sec_param.oob          = SEC_PARAM_OOB;
//     param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
//     param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
//     param.sec_param.kdist_periph.enc = 1;
//     param.sec_param.kdist_periph.id  = 1;

//     err_code = dm_register(&m_dm_app_id,&param);
//     APP_ERROR_CHECK(err_code);
// }






/** @brief Function for the Power manager.
 */
static void power_manage(void) {
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


// /**@brief Heart Rate Collector Handler.
//  */
// static void hrs_c_evt_handler(ble_hrs_c_t * p_hrs_c, ble_hrs_c_evt_t * p_hrs_c_evt)
// {
//     bool     success;
//     uint32_t err_code;

//     switch (p_hrs_c_evt->evt_type)
//     {
//         case BLE_HRS_C_EVT_DISCOVERY_COMPLETE:
//             // Initiate bonding.
//             err_code = dm_security_setup_req(&m_dm_device_handle);
//             APP_ERROR_CHECK(err_code);

//             // Heart rate service discovered. Enable notification of Heart Rate Measurement.
//             err_code = ble_hrs_c_hrm_notif_enable(p_hrs_c);
//             APP_ERROR_CHECK(err_code);

//             success = APPL_LCD_WRITE("Heart Rate", 10, LCD_UPPER_LINE, 0);
//             APP_ERROR_CHECK_BOOL(success);
//             break;

//         case BLE_HRS_C_EVT_HRM_NOTIFICATION:
//         {
//             APPL_LOG("[APPL]: HR Measurement received %d \r\n", p_hrs_c_evt->params.hrm.hr_value);

//             char hr_as_string[LCD_LLEN];

//             sprintf(hr_as_string, "Heart Rate %d", p_hrs_c_evt->params.hrm.hr_value);

//             success = APPL_LCD_WRITE(hr_as_string, strlen(hr_as_string), LCD_UPPER_LINE, 0);
//             APP_ERROR_CHECK_BOOL(success);
//             break;
//         }
//         default:
//             break;
//     }
// }


// /**@brief Battery levelCollector Handler.
//  */
// static void bas_c_evt_handler(ble_bas_c_t * p_bas_c, ble_bas_c_evt_t * p_bas_c_evt)
// {
//     bool     success;
//     uint32_t err_code;

//     switch (p_bas_c_evt->evt_type)
//     {
//         case BLE_BAS_C_EVT_DISCOVERY_COMPLETE:
//             // Batttery service discovered. Enable notification of Battery Level.
//             APPL_LOG("[APPL]: Battery Service discovered. \r\n");

//             APPL_LOG("[APPL]: Reading battery level. \r\n");

//             err_code = ble_bas_c_bl_read(p_bas_c);
//             APP_ERROR_CHECK(err_code);


//             APPL_LOG("[APPL]: Enabling Battery Level Notification. \r\n");
//             err_code = ble_bas_c_bl_notif_enable(p_bas_c);
//             APP_ERROR_CHECK(err_code);

//             break;

//         case BLE_BAS_C_EVT_BATT_NOTIFICATION:
//         {
//             APPL_LOG("[APPL]: Battery Level received %d %%\r\n", p_bas_c_evt->params.battery_level);

//             char bl_as_string[LCD_LLEN];

//             sprintf(bl_as_string, "Battery %d %%", p_bas_c_evt->params.battery_level);

//             success = APPL_LCD_WRITE(bl_as_string, strlen(bl_as_string), LCD_LOWER_LINE, 0);
//             APP_ERROR_CHECK_BOOL(success);
//             break;
//         }

//         case BLE_BAS_C_EVT_BATT_READ_RESP:
//         {
//             APPL_LOG("[APPL]: Battery Level Read as %d %%\r\n", p_bas_c_evt->params.battery_level);

//             char bl_as_string[LCD_LLEN];

//             sprintf(bl_as_string, "Battery %d %%", p_bas_c_evt->params.battery_level);

//             success = APPL_LCD_WRITE(bl_as_string, strlen(bl_as_string), LCD_LOWER_LINE, 0);
//             APP_ERROR_CHECK_BOOL(success);
//             break;
//         }
//         default:
//             break;
//     }
// }


// /**
//  * @brief Heart rate collector initialization.
//  */
// static void hrs_c_init(void)
// {
//     ble_hrs_c_init_t hrs_c_init_obj;

//     hrs_c_init_obj.evt_handler = hrs_c_evt_handler;

//     uint32_t err_code = ble_hrs_c_init(&m_ble_hrs_c, &hrs_c_init_obj);
//     APP_ERROR_CHECK(err_code);
// }


// /**
//  * @brief Battery level collector initialization.
//  */
// static void bas_c_init(void)
// {
//     ble_bas_c_init_t bas_c_init_obj;

//     bas_c_init_obj.evt_handler = bas_c_evt_handler;

//     uint32_t err_code = ble_bas_c_init(&m_ble_bas_c, &bas_c_init_obj);
//     APP_ERROR_CHECK(err_code);
// }


// /**
//  * @brief Database discovery collector initialization.
//  */
// static void db_discovery_init(void)
// {
//     uint32_t err_code = ble_db_discovery_init();
//     APP_ERROR_CHECK(err_code);
// }



// /**@breif Function to start scanning.
//  */
// static void scan_start(void)
// {
//     ble_gap_whitelist_t   whitelist;
//     ble_gap_addr_t        * p_whitelist_addr[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
//     ble_gap_irk_t         * p_whitelist_irk[BLE_GAP_WHITELIST_IRK_MAX_COUNT];
//     uint32_t              err_code;
//     uint32_t              count;

//     // Verify if there is any flash access pending, if yes delay starting scanning until
//     // it's complete.
//     err_code = pstorage_access_status_get(&count);
//     APP_ERROR_CHECK(err_code);

//     if (count != 0)
//     {
//         m_memory_access_in_progress = true;
//         return;
//     }

//     // // Initialize whitelist parameters.
//     // whitelist.addr_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
//     // whitelist.irk_count  = 0;
//     // whitelist.pp_addrs   = p_whitelist_addr;
//     // whitelist.pp_irks    = p_whitelist_irk;

//     // // Request creating of whitelist.
//     // err_code = dm_whitelist_create(&m_dm_app_id,&whitelist);
//     // APP_ERROR_CHECK(err_code);

//     // if (((whitelist.addr_count == 0) && (whitelist.irk_count == 0)) ||
//     //      (m_scan_mode != BLE_WHITELIST_SCAN))
//     // {
//         // No devices in whitelist, hence non selective performed.
//         m_scan_param.active       = 0;            // Active scanning set.
//         m_scan_param.selective    = 0;            // Selective scanning not set.
//         m_scan_param.interval     = SCAN_INTERVAL;// Scan interval.
//         m_scan_param.window       = SCAN_WINDOW;  // Scan window.
//         m_scan_param.p_whitelist  = NULL;         // No whitelist provided.
//         m_scan_param.timeout      = 0x0000;       // No timeout.
//     // }
//     // else
//     // {
//     //     // Selective scanning based on whitelist first.
//     //     m_scan_param.active       = 0;            // Active scanning set.
//     //     m_scan_param.selective    = 1;            // Selective scanning not set.
//     //     m_scan_param.interval     = SCAN_INTERVAL;// Scan interval.
//     //     m_scan_param.window       = SCAN_WINDOW;  // Scan window.
//     //     m_scan_param.p_whitelist  = &whitelist;   // Provide whitelist.
//     //     m_scan_param.timeout      = 0x001E;       // 30 seconds timeout.

//     //     // Set whitelist scanning state.
//     //     m_scan_mode = BLE_WHITELIST_SCAN;
//     // }

//     err_code = sd_ble_gap_scan_start(&m_scan_param);
//     APP_ERROR_CHECK(err_code);

//     // bool lcd_write_status = APPL_LCD_WRITE("Scanning", 8, LCD_UPPER_LINE, 0);
//     // if (!lcd_write_status)
//     // {
//     //     APPL_LOG("[APPL]: LCD Write failed!\r\n");
//     // }

//     // nrf_gpio_pin_set(SCAN_LED_PIN_NO);
// }

int main(void)
{
    // Initialization of various modules.


    platform_init();



    // nrf_gpio_cfg_output(ADV_PIN);
    // nrf_gpio_pin_clear(ADV_PIN);

    // nrf_gpio_cfg_output(3);
    // nrf_gpio_pin_clear(3);

    // led_on(LED_GOT_ADV_PACKET);

    // Setup BLE and services
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    timers_init();
    conn_params_init();

    // Make this a SPI slave
    spi_slave_example_init();


    // device_manager_init();
    // db_discovery_init();


  //  hrs_c_init();
  //  bas_c_init();

    // Start scanning for peripherals and initiate connection
    // with devices that advertise Heart Rate UUID.
    // scan_start();

    // Advertise that we are a TORCH board
    advertising_start();



    // led_on(LED_GOT_ADV_PACKET);

    while (1) {
        power_manage();
    }
}


