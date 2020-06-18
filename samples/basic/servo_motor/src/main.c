/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based servomotor control
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#define PWM_NODE    DT_LABEL(DT_ALIAS(servo_pwm))
#define PWM_CHANNEL DT_PROP(DT_ALIAS(servo_pwm), ch0_pin)

//#define PWM_CHANNEL DT_N_S_soc_S_pwm_4001c000_P_ch0_pin

/*
 * Unlike pulse width, the PWM period is not a critical parameter for
 * motor control. 20 ms is commonly used.
 */
#define PERIOD_USEC	(20U * USEC_PER_MSEC)
#define STEP_USEC	1
#define MIN_PULSE_USEC	2500
#define MAX_PULSE_USEC	5000

void main(void)
{
    struct device *pwm;
    u32_t pulse_width = MIN_PULSE_USEC;
    u8_t dir = 0U;

    k_sleep(K_SECONDS(1));
    printk("Servomotor control\n");

    pwm = device_get_binding(PWM_NODE);
    if (!pwm)
    {
        printk("Cannot find %s!\n", PWM_NODE);
        return;
    }

    while (1) {
        int ret = pwm_pin_set_usec(pwm, PWM_CHANNEL, 16000, pulse_width, 0);
        if (ret) {
            printk("Error %d: failed to set pulse width\n", ret);
            return;
        }

        if (dir) {
            if (pulse_width <= MIN_PULSE_USEC) {
                dir = 0U;
                pulse_width = MIN_PULSE_USEC;
            } else {
                pulse_width -= STEP_USEC;
            }
        } else {
            pulse_width += STEP_USEC;

            if (pulse_width >= MAX_PULSE_USEC) {
                dir = 1U;
                pulse_width = MAX_PULSE_USEC;
            }
        }

        k_sleep(K_MSEC(1));
    }
}
