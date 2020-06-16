#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>
#include "ble.h"
#include "cts.h"

/**** **** **** **** Central API **** **** **** ****/

void main(void)
{
    ble_initialise();
    start_scan();

    while (1) {
        k_sleep(K_SECONDS(1));

        /* Current Time Service updates only when time is changed */
        cts_notify();

    }
}
