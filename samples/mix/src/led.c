#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/led_strip.h>
#include <device.h>
#include <drivers/spi.h>
#include <sys/util.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(main);
#define LOG_LEVEL 4
#define STRIP_ALIAS      DT_ALIAS(led_strip)
#define STRIP_LABEL      DT_LABEL(STRIP_ALIAS)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_ALIAS, chain_length)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

struct device * strip;

void led_strip_init(void)
{
    strip = device_get_binding(STRIP_LABEL);
    if (strip)
    {
        printk("Found LED strip device %s\n", STRIP_LABEL);
    }
    else
    {
        printk("Device %s not found\n", STRIP_LABEL);
    }
}

void do_led_strip(void)
{
    const struct led_rgb colors[] = {
        RGB(0x0f, 0x00, 0x00), /* red */
        RGB(0x00, 0x0f, 0x00), /* green */
        RGB(0x00, 0x00, 0x0f), /* blue */
        RGB(0x0f, 0x0f, 0x00), /* yellow */
        RGB(0x00, 0x0f, 0x0f), /* cyan */
        RGB(0x0f, 0x00, 0x0f), /* magenta */
        RGB(0x00, 0x0f, 0x0f), /* cyan */
        RGB(0x0f, 0x0f, 0x00), /* yellow */
        RGB(0x00, 0x00, 0x0f), /* blue */
        RGB(0x00, 0x0f, 0x00), /* green */
    };

    static struct led_rgb pixels[STRIP_NUM_PIXELS];
    static size_t cursor = 0;
    static size_t color = 0;
    int rc = 0;

    memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));
    rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

    if (rc)
    {
        LOG_ERR("couldn't update strip: %d", rc);
    }

    cursor++;
    if (cursor >= STRIP_NUM_PIXELS)
    {
        cursor = 0;
        color++;
        if (color == ARRAY_SIZE(colors))
        {
            color = 0;
        }
    }
}
