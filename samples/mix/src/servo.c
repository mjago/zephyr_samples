#include <zephyr.h>
#include <drivers/pwm.h>
#include <sys/printk.h>

#define UP              0U
#define DOWN            1U
#define PERIOD_USEC	(20U * USEC_PER_MSEC)
#define STEP_USEC	16
#define MIN_PULSE_USEC	2500
#define MAX_PULSE_USEC	5000

#define PWM_ALIAS        DT_ALIAS(servo_pwm)
#define PWM_LABEL        DT_LABEL(PWM_ALIAS)
#define PWM_CHANNEL      DT_PROP(PWM_ALIAS, ch0_pin)

struct device *pwm;

void servo_init(void)
{
    pwm = device_get_binding(PWM_LABEL);
    if (pwm)
    {
        printk("Found PWM device %s\n", PWM_LABEL);
    }
    else
    {
        printk("Device %s not found\n", PWM_LABEL);
    }
}

void do_servo(void)
{
    static u8_t dir = 0U;
    static u32_t pulse_width = MIN_PULSE_USEC;
    int ret = pwm_pin_set_usec(pwm, PWM_CHANNEL, 16000, pulse_width, 0);

    if (ret) {
        printk("Error %d: failed to set pulse width\n", ret);
        return;
    }

    if (dir) {
        if (pulse_width <= MIN_PULSE_USEC) {
            dir = UP;
            printk("Up\n");
            pulse_width = MIN_PULSE_USEC;
        } else {
            pulse_width -= STEP_USEC;
        }
    } else {
        pulse_width += STEP_USEC;

        if (pulse_width >= MAX_PULSE_USEC) {
            dir = DOWN;
            printk("Down\n");
            pulse_width = MAX_PULSE_USEC;
        }
    }
}
