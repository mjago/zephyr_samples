#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <sys/time.h>
#include <time.h>
//#include <unistd.h>

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value);
static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, u16_t len, u16_t offset);
static ssize_t write_ct(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
			const void *buf,
                        u16_t len,
                        u16_t offset,
			u8_t flags);
static void generate_current_time(u8_t *buf);

static u8_t current_time[10];
static u8_t ct_update;
struct timeval tv;
u8_t buf[10];


/* Current Time Service Declaration */
BT_GATT_SERVICE_DEFINE(
    cts_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_CTS_CURRENT_TIME,
        BT_GATT_CHRC_READ   |
        BT_GATT_CHRC_NOTIFY |
        BT_GATT_CHRC_WRITE,

        BT_GATT_PERM_READ |
        BT_GATT_PERM_WRITE,

        read_ct,
        write_ct,
        current_time),

    BT_GATT_CCC(
        ct_ccc_cfg_changed,
        BT_GATT_PERM_READ |
        BT_GATT_PERM_WRITE),
    );

void cts_init(void)
{
    /* Simulate current time for Current Time Service */
    generate_current_time(current_time);
}

void cts_notify(void)
{
    ct_update = 0U;
    generate_current_time(current_time);
    bt_gatt_notify(NULL, &cts_svc.attrs[1], &current_time, sizeof(current_time));
}

void callback(void)
{
    printk("In callback");
}


//void cts_read(struct bt_conn *conn)
//{
//    int err;
//
//    struct bt_gatt_read_params rd_params;
//    rd_params.func = callback;
//    rd_params.handle_count = 1;
//    rd_params.by_uuid.start_handle = 0x0001;
//    rd_params.by_uuid.end_handle = 0xffff;
//    rd_params.by_uuid.uuid = BT_UUID_CTS_CURRENT_TIME;
//
////    params.start_handle = 0x0001;
////    params.end_handle = 0xffff;
////    params.offset = 0;
//
//    err = bt_gatt_read(conn, &rd_params);
//    if(err)
//    {
//        printk("ERROR\n");
//        for(;;);
//    }
//}

/* Obtain current time. */
static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                               u16_t value)
{
	/* TODO: Handle value */
}

static ssize_t read_ct(struct bt_conn *conn,
                       const struct bt_gatt_attr *attr,
		       void *buf,
                       u16_t len,
                       u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(current_time));
}

static ssize_t write_ct(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
			const void *buf,
                        u16_t len,
                        u16_t offset,
			u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(current_time)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);
    ct_update = 1U;

    return len;
}

static void generate_current_time(u8_t *buf)
{
    u16_t year;
    struct tm *nowtm;
    int res = gettimeofday(&tv, NULL);

    if (res < 0) {
        printk("Error in gettimeofday(): %d\n", errno);
    }
    else
    {
        nowtm = localtime(&tv.tv_sec);
        printk ("Ctrl: %02u:", nowtm->tm_hour);
        printk ("%02u:", nowtm->tm_min);
        printk ("%02u\n", nowtm->tm_sec);

        year = sys_cpu_to_le16(nowtm->tm_year + 1900);
        memcpy(buf,  &year, 2);      /* year */
        buf[2] = nowtm->tm_mon + 1;  /* months starting from 1 */
        buf[3] = nowtm->tm_mday;     /* day */
        buf[4] = nowtm->tm_hour;     /* hours */
        buf[5] = nowtm->tm_min;      /* minutes */
        buf[6] = nowtm->tm_sec;      /* seconds */
        buf[7] = 1U;                 /* day of week starting from 1 */
        buf[8] = 0U;                 /* 'Fractions 256 part of 'Exact Time 256' */
        buf[9] = 0U;                 /* Adjust reason */
    }
}

