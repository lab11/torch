#include "contiki-net.h"
#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sys-ctrl.h"
#include "dev/pwm.h"
#include "net/rime/broadcast.h"
#include "net/ip/uip.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ip/uiplib.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "rest-engine.h"


#define SW_VERSION "1.0"
#define HW_VERSION "A"


PROCESS(app, "SDL Torch with CoAP");
AUTOSTART_PROCESSES(&app);

uint8_t  light_on = 1;
uint32_t light_freq = 2000; // 2 kHz
uint32_t light_dc = 50; // 50%



static void
light_set_on ()
{
  if (light_on) return;
  leds_toggle(LEDS_GREEN);
  pwm_init(GPTIMER_2, GPTIMER_SUBTIMER_A, light_freq, LED_PWM_PORT_NUM, LED_PWM_PIN);
  pwm_start(GPTIMER_2, GPTIMER_SUBTIMER_A);
  light_on = 1;
}

static void
light_set_frequency (uint32_t freq)
{
  light_freq = freq;
  light_on = 0;
  light_set_on(freq);
}

static void
light_set_dc (uint32_t dc)
{
  light_dc = dc;
}

static void
light_set_off ()
{
  if (!light_on) return;
  pwm_stop(GPTIMER_2, GPTIMER_SUBTIMER_A);
  GPIO_SOFTWARE_CONTROL(GPIO_PORT_TO_BASE(LED_PWM_PORT_NUM), GPIO_PIN_MASK(LED_PWM_PIN));
  GPIO_SET_OUTPUT(GPIO_PORT_TO_BASE(LED_PWM_PORT_NUM), GPIO_PIN_MASK(LED_PWM_PIN));
  GPIO_CLR_PIN(GPIO_PORT_TO_BASE(LED_PWM_PORT_NUM), GPIO_PIN_MASK(LED_PWM_PIN));
  light_on = 0;
}





#define TO_CHAR(X) (((X) < 10) ? ('0' + (X)) : ('a' + ((X) - 10)))
int inet_ntop6 (const uip_ipaddr_t *addr, char *buf, int cnt) {
  uint16_t block, block2;
  char *end = buf + cnt;
  int i, j, compressed = 0;

  for (j = 0; j < 8; j++) {
    if (buf > end - 8)
      goto done;

    //block = ntohs(addr->s6_addr16[j]);
    block = (addr->u8[j*2] << 8) + addr->u8[(j*2) + 1];
    block2 = block;
    for (i = 4; i <= 16; i+=4) {
      if (block > (0xffff >> i) || (compressed == 2 && i == 16)) {
        *buf++ = TO_CHAR((block >> (16 - i)) & 0xf);
      }
    }
    if (block2 == 0 && compressed == 0) {
      *buf++ = ':';
      compressed++;
    }
    if (block2 != 0 && compressed == 1) compressed++;

    if (j < 7 && compressed != 1) *buf++ = ':';
  }
  if (compressed == 1)
    *buf++ = ':';
 done:
  *buf++ = '\0';
  return buf - (end - cnt);
}


int
coap_parse_bool (void* request)
{
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    if (strncmp(payload, "true", length) == 0) {
      return 1;
    } else if (strncmp(payload, "false", length) == 0) {
      return 0;
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

int
coap_parse_uint (void* request, uint32_t* u)
{
  int length;
  const char* payload = NULL;
  // int n;

  // *u = strtol()

  length = REST.get_request_payload(request, (const uint8_t**) &payload);


  if (length > 0) {
    *u = strtol(payload, NULL, 10);
    // n = 1;
    // *u = 9;
    // //n = sscanf(payload, "%u", (unsigned int*) u);
    // if (n == 1) {
      return 0;
    // }
  }

  return -1;
}



/*******************************************************************************
 * onoff/Power
 ******************************************************************************/

static void
onoff_power_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s", (light_on)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
onoff_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  int b = coap_parse_bool(request);
  if (b == 1) {
    light_set_on();
  } else if (b == 0) {
    light_set_off();
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_onoff_power,
         "title=\"onoffdevice/Power\";rt=\"Light\"",
         onoff_power_get_handler,
         onoff_power_post_handler,
         onoff_power_post_handler,
         NULL);


/*******************************************************************************
 * led0/Power
 ******************************************************************************/

static void
led0_power_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s",
    ((leds_get()&LEDS_GREEN)==LEDS_GREEN)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
led0_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint8_t b = coap_parse_bool(request);
  if (b == 1) {
    leds_on(LEDS_GREEN);
  } else if (b == 0) {
    leds_off(LEDS_GREEN);
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

/* A simple actuator example. Toggles the red led */
RESOURCE(coap_led0_power,
         "title=\"Green LED\";rt=\"Control\"",
         led0_power_get_handler,
         led0_power_post_handler,
         led0_power_post_handler,
         NULL);


/*******************************************************************************
 * led1/Power
 ******************************************************************************/

static void
led1_power_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s",
    ((leds_get()&LEDS_RED)==LEDS_RED)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
led1_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint8_t b = coap_parse_bool(request);
  if (b == 1) {
    leds_on(LEDS_RED);
  } else if (b == 0) {
    leds_off(LEDS_RED);
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

/* A simple actuator example. Toggles the red led */
RESOURCE(coap_led1_power,
         "title=\"Red LED\";rt=\"Control\"",
         led1_power_get_handler,
         led1_power_post_handler,
         led1_power_post_handler,
         NULL);


/*******************************************************************************
 * led2/Power
 ******************************************************************************/

static void
led2_power_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s",
    ((leds_get()&LEDS_BLUE)==LEDS_BLUE)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
led2_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint8_t b = coap_parse_bool(request);
  if (b == 1) {
    leds_on(LEDS_BLUE);
  } else if (b == 0) {
    leds_off(LEDS_BLUE);
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

/* A simple actuator example. Toggles the red led */
RESOURCE(coap_led2_power,
         "title=\"Orange LED\";rt=\"Control\"",
         led2_power_get_handler,
         led2_power_post_handler,
         led2_power_post_handler,
         NULL);

/*******************************************************************************
 * sdl/luxapose/Frequency
 ******************************************************************************/

static void
sdl_luxapose_frequency_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%u", (unsigned int) light_freq);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
sdl_luxapose_frequency_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint32_t u;
  int ret;

  ret = coap_parse_uint(request, &u);
  if (ret < 0) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    light_set_frequency(u);
  }
}

RESOURCE(coap_sdl_luxapose_frequency,
         "title=\"Luxapose Frequency\"",
         sdl_luxapose_frequency_get_handler,
         sdl_luxapose_frequency_post_handler,
         sdl_luxapose_frequency_post_handler,
         NULL);

/*******************************************************************************
 * sdl/luxapose/DutyCycle
 ******************************************************************************/

static void
sdl_luxapose_dutycycle_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%u", (unsigned int) light_dc);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
sdl_luxapose_dutycycle_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint32_t u;
  int ret;

  ret = coap_parse_uint(request, &u);
  if (ret < 0) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    light_set_dc(u);
  }
}

RESOURCE(coap_sdl_luxapose_dutycycle,
         "title=\"Luxapose Duty Cycle (todo)\"",
         sdl_luxapose_dutycycle_get_handler,
         sdl_luxapose_dutycycle_post_handler,
         sdl_luxapose_dutycycle_post_handler,
         NULL);



/*******************************************************************************
 * device/software/Version
 ******************************************************************************/

static void
device_software_version_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "%s";

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, SW_VERSION);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_device_software_version,
         "title=\"device/software/Version\";rt=\"sw\"",
         device_software_version_get_handler,
         NULL,
         NULL,
         NULL);


/*******************************************************************************
 * device/hardware/Version
 ******************************************************************************/

static void
device_hardware_version_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "%s";

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, HW_VERSION);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_device_hardware_version,
         "title=\"device/hardware/Version\";rt=\"hw\"",
         device_hardware_version_get_handler,
         NULL,
         NULL,
         NULL);







PROCESS_THREAD(app, ev, data) {
  PROCESS_BEGIN();

  leds_on(LEDS_ALL);

  // CoAP + REST
  rest_init_engine();

  rest_activate_resource(&coap_onoff_power,  "onoff/Power");

  rest_activate_resource(&coap_led0_power,         "led0/Power");
  rest_activate_resource(&coap_led1_power,         "led1/Power");
  rest_activate_resource(&coap_led2_power,         "led2/Power");

  rest_activate_resource(&coap_sdl_luxapose_frequency,      "sdl/luxapose/Frequency");
  rest_activate_resource(&coap_sdl_luxapose_dutycycle,      "sdl/luxapose/DutyCycle");

  rest_activate_resource(&coap_device_software_version, "device/software/Version");
  rest_activate_resource(&coap_device_hardware_version, "device/hardware/Version");


  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}