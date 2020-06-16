/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

static struct bt_conn *default_conn;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
	}
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
    printk("Discover Func\n");
//	int err;
//
//	if (!attr) {
//		printk("Discover complete\n");
//		(void)memset(params, 0, sizeof(*params));
//		return BT_GATT_ITER_STOP;
//	}
//
//	printk("[ATTRIBUTE] handle %u\n", attr->handle);
//
//	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS)) {
//		memcpy(&uuid, BT_UUID_HRS_MEASUREMENT, sizeof(uuid));
//		discover_params.uuid = &uuid.uuid;
//		discover_params.start_handle = attr->handle + 1;
//		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
//
//		err = bt_gatt_discover(conn, &discover_params);
//		if (err) {
//			printk("Discover failed (err %d)\n", err);
//		}
//	} else if (!bt_uuid_cmp(discover_params.uuid,
//				BT_UUID_HRS_MEASUREMENT)) {
//		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
//		discover_params.uuid = &uuid.uuid;
//		discover_params.start_handle = attr->handle + 2;
//		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
//		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
//
//		err = bt_gatt_discover(conn, &discover_params);
//		if (err) {
//			printk("Discover failed (err %d)\n", err);
//		}
//	} else {
//		subscribe_params.notify = notify_func;
//		subscribe_params.value = BT_GATT_CCC_NOTIFY;
//		subscribe_params.ccc_handle = attr->handle;
//
//		err = bt_gatt_subscribe(conn, &subscribe_params);
//		if (err && err != -EALREADY) {
//			printk("Subscribe failed (err %d)\n", err);
//		} else {
//			printk("[SUBSCRIBED]\n");
//		}
//
//		return BT_GATT_ITER_STOP;
//	}
//
//	return BT_GATT_ITER_STOP;
}


static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
            return;
	}

        memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.func = discover_func;
        discover_params.start_handle = 0x0001;
        discover_params.end_handle = 0xffff;
        discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(default_conn, &discover_params);
        if (err) {
            printk("Discover failed(err %d)\n", err);
            return;
        }

	printk("Connected: %s\n", addr);

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_scan();
}
