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
#include "cts.h"

/**** **** **** **** Static Prototypes **** **** **** ****/

static u8_t notify_func(struct bt_conn *conn,
                        struct bt_gatt_subscribe_params *params,
                        const void *data, u16_t length);
static u8_t discover_cts_fn(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params);
static bool eir_found(struct bt_data *data,
                      void *user_data);
static void device_found(const bt_addr_le_t *addr,
                         s8_t rssi,
                         u8_t type,
                         struct net_buf_simple *ad);
static void connected_cb(struct bt_conn *conn, u8_t conn_err);
static void disconnected_cb(struct bt_conn *conn, u8_t reason);
static void bt_ready(void);

/**** **** **** **** Local Variables **** **** **** ****/

static struct bt_conn *default_conn;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_conn_cb conn_callbacks = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
};

/**** **** **** **** BLE API **** **** **** ****/

void ble_initialise(void)
{
    int err = bt_enable(NULL);

    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");
    bt_ready();
    bt_conn_cb_register(&conn_callbacks);
}

void start_scan(void)
{
    int err;

    /* Use active scanning and disable duplicate filtering to handle any
     * devices that might update their advertising data at runtime. */
    struct bt_le_scan_param scan_param = {
        .type       = BT_LE_SCAN_TYPE_ACTIVE,
        .options    = BT_LE_SCAN_OPT_NONE,
        .interval   = BT_GAP_SCAN_FAST_INTERVAL,
        .window     = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err)
    {
        printk("Scanning failed to start (err %d)\n", err);
    }
    else
    {
        printk("Scanning successfully started\n");
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
        u8_t mon;     /* months starting from 1 */
        u8_t mday;    /* day */
        u8_t hour;    /* hours */
        u8_t min;     /* minutes */
        u8_t sec;     /* seconds */
        u8_t frac256; /* fraction */
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

static u8_t discover_cts_fn(struct bt_conn *conn,
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
        discover_params.start_handle = attr->handle +a 1;
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

static bool eir_found(struct bt_data *data, void *user_data)
{
    bool ret = true;

    printk("[AD]: %u data_len %u\n", data->type, data->data_len);

    switch (data->type)
    {
        /* Intentional Fallthrough */
    case BT_DATA_UUID16_SOME:
    case BT_DATA_UUID16_ALL:
    {
        struct bt_le_conn_param *param;
        struct bt_uuid *uuid;
        bt_addr_le_t *addr = user_data;
        u16_t u16;
        int err;
        int i;

        if (data->data_len % sizeof(u16_t) != 0U) {
            printk("AD malformed\n");
            return true;
        }

        for (i = 0; i < data->data_len; i += sizeof(u16_t)) {
            memcpy(&u16, &data->data[i], sizeof(u16));
            uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));

//            if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
//                continue;
//            }

//            if (bt_uuid_cmp(uuid, BT_UUID_CTS)) {
//                continue;
//            }

            err = bt_le_scan_stop();
            if (err) {
                printk("Stop LE scan failed (err %d)\n", err);
                continue;
            }

            param = BT_LE_CONN_PARAM_DEFAULT;
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                    param, &default_conn);
            if (err) {
                printk("Create conn failed (err %d)\n", err);
            }

//            cts_read(default_conn);
            return false;
        }
    }
    }

    return ret;
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
                         struct net_buf_simple *ad)
{
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
           dev, type, ad->len, rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        bt_data_parse(ad, eir_found, (void *)addr);
    }
}

static void connected_cb(struct bt_conn *conn, u8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        printk("Failed to connect to %s (%u)\n", addr, conn_err);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    printk("Connected: %s\n", addr);

    if (conn == default_conn) {
        memcpy(&uuid, BT_UUID_CTS, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.func = discover_cts_fn;
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
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

    if (default_conn != conn) {
        return;
    }

    bt_conn_unref(default_conn);
    default_conn = NULL;
    start_scan();
}

static void bt_ready(void)
{
    printk("Bluetooth initialized\n");
    cts_init();
}

/*
  bt_conn:          connection
  discovery_params: params
 */

//int bt_gatt_discover(struct bt_conn *conn,
//                     struct bt_gatt_discover_params *params)
//{
//    int ret = -EINVAL;
//
//    if(conn->state != BT_CONN_CONNECTED)
//    {
//        ret = -ENOTCONN;
//    }
//    else
//    {
//        switch (params->type)
//        {
//            /* Intentional Fallthrough */
//        case BT_GATT_DISCOVER_PRIMARY:
//        case BT_GATT_DISCOVER_SECONDARY:
//            ret = ((params->uuid)                ?
//                   gatt_find_type(conn, params)  :
//                   gatt_read_group(conn, params));
//            break;
//
//            /* Intentional Fallthrough */
//        case BT_GATT_DISCOVER_INCLUDE:
//        case BT_GATT_DISCOVER_CHARACTERISTIC:
//            ret = gatt_read_type(conn, params);
//            break;
//
//        case BT_GATT_DISCOVER_DESCRIPTOR:
//            /* Only descriptors can be filtered */
//            if (params->uuid &&
//                ( ! bt_uuid_cmp(params->uuid, BT_UUID_GATT_PRIMARY)   ||
//                  ! bt_uuid_cmp(params->uuid, BT_UUID_GATT_SECONDARY) ||
//                  ! bt_uuid_cmp(params->uuid, BT_UUID_GATT_INCLUDE)   ||
//                  ! bt_uuid_cmp(params->uuid, BT_UUID_GATT_CHRC))      )
//            {
//                break;
//            }
//
//            /* Fallthrough. */
//        case BT_GATT_DISCOVER_ATTRIBUTE:
//            ret = gatt_find_info(conn, params);
//            break;
//
//        default:
//            BT_ERR("Invalid discovery type: %u", params->type);
//            break;
//        }
//    }
//
//    return ret;
//}
