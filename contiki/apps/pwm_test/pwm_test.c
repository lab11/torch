
#include "contiki.h"
#include "sys/etimer.h"
#include "dev/leds.h"
#include "dev/gptimer.h"

static struct etimer periodic_timer_red;
// static struct etimer periodic_timer_green;
// static struct etimer periodic_timer_blue;

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "PWM Test");
AUTOSTART_PROCESSES(&test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data) {

	PROCESS_BEGIN();

	pwm_init(GPTIMER_2, GPTIMER_SUBTIMER_A, 0, LED_PWM_PORT_NUM, LED_PWM_PIN);
	pwm_start(GPTIMER_2, GPTIMER_SUBTIMER_A);

	leds_on(LEDS_ALL);

	etimer_set(&periodic_timer_red, CLOCK_SECOND);
	// etimer_set(&periodic_timer_green, CLOCK_SECOND/2);
	// etimer_set(&periodic_timer_blue, CLOCK_SECOND/4);

	while(1) {
		PROCESS_YIELD();

		if (etimer_expired(&periodic_timer_red)) {
			printf("Red\n");
			leds_toggle(LEDS_RED);
			leds_off(LEDS_BLUE);
			etimer_restart(&periodic_timer_red);
		}
		//else if (etimer_expired(&periodic_timer_green)) {
		// 	leds_toggle(LEDS_GREEN);
		// 	etimer_restart(&periodic_timer_green);
		// } else if (etimer_expired(&periodic_timer_blue)) {
		// 	leds_toggle(LEDS_BLUE);
		// 	etimer_restart(&periodic_timer_blue);
		// }
	}

	PROCESS_END();
}
