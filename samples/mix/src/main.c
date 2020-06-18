#include <zephyr.h>
#include <sys/printk.h>
#include "servo.h"
#include "led.h"

#define DELAY_TIME       K_MSEC(16)

void main(void)
{

    k_sleep(K_SECONDS(1));

    servo_init();
    led_strip_init();

    for(;;) {
        do_servo();
        do_led_strip();
        k_sleep(DELAY_TIME);
    }
}
