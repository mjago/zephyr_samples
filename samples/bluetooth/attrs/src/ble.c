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
#include "debug_ad_data_type.h"
#define DISCOVERED 0

#include <zephyr.h>
#include <stddef.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

// #define BT_UUID_FITNESS_MACHINE BT_UUID_DECLARE_16(0x1826)

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, u16_t len, u16_t offset);
static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, u16_t len, u16_t offset,
                          u8_t flags);

static struct bt_uuid_128 tended = BT_UUID_INIT_128(BT_UUID_128_ENCODE  \
                                                    (0x00001891, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 fall = BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x00003005, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 nomove =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x00003007, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 other =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x0000300a, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 notif =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x0000300c, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 autoci =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x0000300d, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 id =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x0000300e, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 uart =  BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x00002e29, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 uarttx = BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x00002e2a, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
static struct bt_uuid_128 uartrx = BT_UUID_INIT_128(BT_UUID_128_ENCODE \
          (0x00002e2b, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));

static u8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

static struct bt_gatt_attr test_attrs[] = {
    /* Vendor Primary Service Declaration */
    BT_GATT_PRIMARY_SERVICE(&tended),

    BT_GATT_CHARACTERISTIC(&fall.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&nomove.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&other.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&notif.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&autoci.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&id.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&uart.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&uarttx.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),

    BT_GATT_CHARACTERISTIC(&uartrx.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                           BT_GATT_PERM_WRITE_AUTHEN,
                           read_test, write_test, test_value),
};

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, u16_t len, u16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             strlen(value));
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *buf, u16_t len, u16_t offset,
                          u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(test_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);
    return len;
}

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

void gatt_register(void)
{
    /* Attempt to register services */
    bt_gatt_service_register(&test_svc);
}
