/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 *
 * This app uses PWM[0].
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#if defined(DT_ALIAS_PWM_LED0_PWMS_CONTROLLER) && defined(DT_ALIAS_PWM_LED0_PWMS_CHANNEL)
/* get the defines from dt (based on alias 'pwm-led0') */
#define PWM_DRIVER	DT_ALIAS_PWM_LED0_PWMS_CONTROLLER
#define PWM_CHANNEL	DT_ALIAS_PWM_LED0_PWMS_CHANNEL
#ifdef DT_ALIAS_PWM_LED0_PWMS_FLAGS
#define PWM_FLAGS	DT_ALIAS_PWM_LED0_PWMS_FLAGS
#else
#define PWM_FLAGS	0
#endif
#else
#error "Choose supported PWM driver"
#endif

/*
 * 50 is flicker fusion threshold. Modulated light will be perceived
 * as steady by our eyes when blinking rate is at least 50.
 */
#define PERIOD (USEC_PER_SEC / 50U)

/* in micro second */
#define FADESTEP	50

#define DELAY_TIME K_MSEC(1)
#define ONE_WAY false

void main(void)
{
    struct device *pwm_dev;
    u32_t pulse_width = 0U;
    u8_t dir = 0U;

    k_sleep(K_MSEC(100));
    printk("PWM demo app-fade LED\n");

    pwm_dev = device_get_binding(PWM_DRIVER);
    if (!pwm_dev)
    {
        printk("Cannot find %s!\n", PWM_DRIVER);
        return;
    }

    while (1)
    {
        if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
                             PERIOD, pulse_width, PWM_FLAGS))
        {
            printk("pwm pin set fails\n");
            return;
        }

        if (dir)
        {
            if (pulse_width < FADESTEP)
            {
                dir = 0U;
                pulse_width = 0U;
            }
            else
            {
                pulse_width -= FADESTEP;
            }
        }
        else
        {
            pulse_width += FADESTEP;

            if (pulse_width >= PERIOD)
            {
                if(! ONE_WAY)
                {
                    dir = 1U;
                    pulse_width = PERIOD;
                }
                else
                {
                    pulse_width = 0;
                    if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
                                         PERIOD, pulse_width, PWM_FLAGS))
                    {
                        printk("pwm pin set fails\n");
                        return;
                    }
                    k_sleep(K_MSEC(500));
                }
            }
        }
        k_sleep(DELAY_TIME);
    }
}
