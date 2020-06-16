/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>

#define OFF 0
#define ON  1

typedef struct LED_DATA
{
    struct device * dev;
    const  char *   ctrlr;
    const  int      pin;
    const  int      cfg;
    const  char *   label;
} led_data_t;

typedef struct BUTTON_DATA
{
    struct device * dev;
    const  char *   ctrlr;
    const  int      pin;
    const  int      cfg;
    const  char *   label;
} button_data_t;

led_data_t leds[4] =
{
    {
        .ctrlr = DT_ALIAS_LED0_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_LED0_GPIOS_PIN,
        .cfg   = DT_ALIAS_LED0_GPIOS_FLAGS | GPIO_OUTPUT,
        .label = DT_ALIAS_LED0_LABEL
    },{
        .ctrlr = DT_ALIAS_LED1_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_LED1_GPIOS_PIN,
        .cfg   = DT_ALIAS_LED1_GPIOS_FLAGS | GPIO_OUTPUT,
        .label = DT_ALIAS_LED1_LABEL
    },{
        .ctrlr = DT_ALIAS_LED2_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_LED2_GPIOS_PIN,
        .cfg   = DT_ALIAS_LED2_GPIOS_FLAGS | GPIO_OUTPUT,
        .label = DT_ALIAS_LED2_LABEL
    },{
        .ctrlr = DT_ALIAS_LED3_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_LED3_GPIOS_PIN,
        .cfg   = DT_ALIAS_LED3_GPIOS_FLAGS | GPIO_OUTPUT,
        .label = DT_ALIAS_LED3_LABEL
    }
};

button_data_t buttons[4] =
{
    {
        .ctrlr = DT_ALIAS_SW0_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_SW0_GPIOS_PIN,
        .cfg   = DT_ALIAS_SW0_GPIOS_FLAGS | GPIO_INPUT,
        .label = DT_ALIAS_SW0_LABEL
    },{
        .ctrlr = DT_ALIAS_SW1_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_SW1_GPIOS_PIN,
        .cfg   = DT_ALIAS_SW1_GPIOS_FLAGS | GPIO_INPUT,
        .label = DT_ALIAS_SW1_LABEL
    },{
        .ctrlr = DT_ALIAS_SW2_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_SW2_GPIOS_PIN,
        .cfg   = DT_ALIAS_SW2_GPIOS_FLAGS | GPIO_INPUT,
        .label = DT_ALIAS_SW2_LABEL
    },{
        .ctrlr = DT_ALIAS_SW3_GPIOS_CONTROLLER,
        .pin   = DT_ALIAS_SW3_GPIOS_PIN,
        .cfg   = DT_ALIAS_SW3_GPIOS_FLAGS | GPIO_INPUT,
        .label = DT_ALIAS_SW3_LABEL
    },
};

typedef enum BUTTON_STATE
{
    BUTTON_OFF,
    BUTTON_ON,
} button_state_t;

static button_state_t button_state[4] = { BUTTON_OFF };
static struct gpio_callback button_cb_data[4];
static void init_leds(void);
static void init_buttons(void);
static void do_button(void);
static void button_intr0(struct device *dev, struct gpio_callback *cb, u32_t pins);
static void button_intr1(struct device *dev, struct gpio_callback *cb, u32_t pins);
static void button_intr2(struct device *dev, struct gpio_callback *cb, u32_t pins);
static void button_intr3(struct device *dev, struct gpio_callback *cb, u32_t pins);
static void do_button_state(int button_count);
static void ( * button_intr[4])(struct device *dev, struct gpio_callback *cb, u32_t pins) = { button_intr0, button_intr1, button_intr2, button_intr3 };

int main(void)
{
    u32_t now;
    u32_t then = 0;
    int   count;

    init_leds();
    init_buttons();

    for(;;)
    {
        now = k_cycle_get_32() / 32768;
        if( then != now)
        {
            then = now;
            printk("Now %" PRIu32 "\n", now);
        }

        for(count = 0; count < 4; count++)
        {
            if(button_state[count] == BUTTON_ON)
            {
                do_button();
                button_state[count] = OFF;
                k_msleep(100);
            }
        }
    }
}

static void init_buttons(void)
{
    int ret;
    int count;

    for(count = 0; count < 4; count++)
    {
        buttons[count].dev = device_get_binding(buttons[count].ctrlr);
        if(buttons[count].dev == NULL) {
            printk("Error: didn't find %s device\n",
                   buttons[count].ctrlr);
            return;
        }
        ret = gpio_pin_configure(buttons[count].dev, buttons[count].pin, buttons[count].cfg);

        if (ret != 0) {
            printk("Error %d: failed to configure pin %d '%s'\n",
                   ret, buttons[count].pin, buttons[count].label);
            return;
        }

        ret = gpio_pin_interrupt_configure(buttons[count].dev, buttons[count].pin,
                                           GPIO_INT_EDGE_TO_ACTIVE);
        if (ret != 0) {
            printk("Error %d: failed to configure interrupt on pin %d '%s'\n",
                   ret, buttons[count].pin, buttons[count].label);
            return;
        }
        gpio_init_callback(&button_cb_data[count], button_intr[count],
                           BIT(buttons[count].pin));
        gpio_add_callback(buttons[count].dev, &button_cb_data[count]);
    }
}

static void init_leds(void)
{
    int ret = 0;
    int count;
    int count2;

    for(count = 0; count < 4; count++)
    {
        leds[count].dev = device_get_binding(leds[count].ctrlr);
        if (leds[count].dev == NULL) {
            printk("Error: didn't find %s device\n",
                   leds[count].ctrlr);
            return;
        }
        ret = gpio_pin_configure(leds[count].dev, leds[count].pin, leds[count].cfg);
        if (ret != 0) {
            printk("Error %d: failed to configure pin %d '%s'\n",
                   ret, leds[count].pin, leds[count].label);
            return;
        }

    }
    for(count2 = 0; count2 < 8; count2++)
    {
        for(count = 0; count < 4; count++)
        {
            if(count2 & 0x01)
            {
                gpio_pin_set(leds[count].dev, leds[count].pin, OFF);
            }
            else
            {
                gpio_pin_set(leds[count].dev, leds[count].pin, ON);
            }
            k_msleep(30);
        }
    }
}

static void button_intr0(struct device *dev, struct gpio_callback *cb, u32_t pins)
{
    do_button_state(0);
}

static void button_intr1(struct device *dev, struct gpio_callback *cb, u32_t pins)
{
    do_button_state(1);
}

static void button_intr2(struct device *dev, struct gpio_callback *cb, u32_t pins)
{
    do_button_state(2);
}

static void button_intr3(struct device *dev, struct gpio_callback *cb, u32_t pins)
{
    do_button_state(3);
}

static void do_button_state(int button_count)
{
    button_data_t btn = buttons[button_count];
    static u32_t time = 0;
    int on_off = gpio_pin_get(btn.dev, btn.pin);
    u32_t now = k_cycle_get_32();

    if(on_off == ON)
    {
        switch(button_state[button_count])
        {
        case BUTTON_OFF:
            printk("%s pressed at %" PRIu32 "\n", btn.label, now / 32768);
            time = now;
            button_state[button_count] = BUTTON_ON;
            break;

        default:
            break;
        }
    }
}

static void do_button(void)
{
    int count;

    for(count = 0; count < 4; count++)
    {
        gpio_pin_set(leds[count].dev, leds[count].pin, ON);
        k_msleep(50);
    }

    for(count = 0; count < 4; count++)
    {
        gpio_pin_set(leds[count].dev, leds[count].pin, OFF);
        k_msleep(50);
    }
}
