#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>
#include "cts.h"

/**** **** **** **** Static Prototypes **** **** **** ****/

static u8_t notify_func(struct bt_conn *conn,
                        struct bt_gatt_subscribe_params *params,
                        const void *data, u16_t length);
static u8_t discover_cts_cb(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params);
static void connected_cb(struct bt_conn *conn, u8_t err);
static void disconnected_cb(struct bt_conn *conn, u8_t reason);
static void bt_ready(void);
static void auth_cancel(struct bt_conn *conn);
static void bas_notify(void);

/**** **** **** **** Local Variables **** **** **** ****/

struct bt_conn *default_conn;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS,
                  (BT_LE_AD_GENERAL |
                   BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  0x0d, 0x18, 0x0f, 0x18, 0x0a, 0x18),
};
static struct bt_conn_cb conn_callbacks = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
};
static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
};

/**** **** **** **** Peripheral API **** **** **** ****/

void main(void)
{
    int err;

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_ready();

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);

    /* Implement notification. At the moment there is no suitable way
     * of starting delayed work so we do it here
     */
    while (1) {
        k_sleep(K_SECONDS(1));

        /* Battery level simulation */
        bas_notify();

        /* Current Time Service updates only when time is changed */
        cts_notify();
    }
}

/**** **** **** **** Static Functions **** **** **** ****/

static u8_t notify_func(struct bt_conn *conn,
                        struct bt_gatt_subscribe_params *params,
                        const void *data, u16_t length)
{
    typedef struct MY_TIME_T {
        u8_t year_0;
        u8_t year_1;
	u8_t mon;  /* months starting from 1 */
	u8_t mday; /* day */
	u8_t hour; /* hours */
	u8_t min;  /* minutes */
	u8_t sec;  /* seconds */
	u8_t f256; /* fraction */
    } my_time_t;

    my_time_t * p_my_time = (my_time_t *) data;;

    if (!data)
    {
        printk("[UNSUBSCRIBED]\n");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    printk("[NOTIFICATION] leng: %u, ", length);
    printk("Time: %02u:", p_my_time->hour);
    printk("%02u:", p_my_time->min);
    printk("%02u\n", p_my_time->sec);
    return BT_GATT_ITER_CONTINUE;
}

static u8_t discover_cts_cb(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        printk("Discover complete\n");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    printk("[CTS ATTRIBUTE] handle %u\n", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_CTS)) {
        printk("Discover BT_UUID_CTS\n");
        memcpy(&uuid, BT_UUID_CTS_CURRENT_TIME, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid,
                            BT_UUID_CTS_CURRENT_TIME)) {
        memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 2;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else {
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err && err != -EALREADY) {
            printk("Subscribe failed (err %d)\n", err);
        } else {
            printk("[SUBSCRIBED CTS]\n");
        }

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static void connected_cb(struct bt_conn *conn, u8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    } else {
        default_conn = bt_conn_ref(conn);
        printk("Connected\n");

        memcpy(&uuid, BT_UUID_CTS, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.func = discover_cts_cb;
        discover_params.start_handle = 0x0001;
        discover_params.end_handle = 0xffff;
        discover_params.type = BT_GATT_DISCOVER_PRIMARY;
        err = bt_gatt_discover(default_conn, &discover_params);
        if (err) {
            printk("Discover failed(err %d)\n", err);
            return;
        }
    }
}

static void disconnected_cb(struct bt_conn *conn, u8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);

    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
}

static void bt_ready(void)
{
    int err;

    printk("Bluetooth initialized\n");

    cts_init();

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static void bas_notify(void)
{
    u8_t battery_level = bt_gatt_bas_get_battery_level();

    battery_level--;
    if (!battery_level)
    {
        battery_level = 100U;
    }
    bt_gatt_bas_set_battery_level(battery_level);
}

