/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include <drivers/led_strip.h>
#include <device.h>
#include <drivers/spi.h>
#include <sys/util.h>

#define STRIP_LABEL		DT_ALIAS_LED_STRIP_LABEL
#define STRIP_NUM_PIXELS	DT_ALIAS_LED_STRIP_CHAIN_LENGTH

#define DELAY_TIME K_MSEC(100)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb colors[] = {
	RGB(0x0f, 0x00, 0x00), /* red */
	RGB(0x0f, 0x0f, 0x00), /* yellow */
	RGB(0x00, 0x0f, 0x00), /* green */
	RGB(0x00, 0x0f, 0x0f), /* cyan */
	RGB(0x00, 0x00, 0x0f), /* blue */
	RGB(0x0f, 0x00, 0x0f), /* magenta */
};

struct led_rgb pixels[STRIP_NUM_PIXELS];

void main(void)
{
	struct device *strip;
	size_t cursor = 0, color = 0;
	int rc;

        k_sleep(K_SECONDS(1));
        printk("here\n");
	strip = device_get_binding(STRIP_LABEL);
	if (strip) {
		LOG_INF("Found LED strip device %s", STRIP_LABEL);
	} else {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return;
	}

	LOG_INF("Displaying pattern on strip");
	while (1) {
            //	memset(&pixels, 0x00, sizeof(pixels));
		memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));
		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

		if (rc) {
			LOG_ERR("couldn't update strip: %d", rc);
		}

		cursor++;
		if (cursor >= STRIP_NUM_PIXELS) {
			cursor = 0;
			color++;
			if (color == ARRAY_SIZE(colors)) {
				color = 0;
			}
		}

		k_sleep(DELAY_TIME);
	}
}
