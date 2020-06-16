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
#include <bluetooth/services/bas.h>
#include <sys/byteorder.h>
#include <unistd.h>
#include <stdio.h>
#include "ble.h"
#include "cts.h"


#include <zephyr.h>
#include <stddef.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

void main(void)
{
    gatt_register();
}
