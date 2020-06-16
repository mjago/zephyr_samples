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
#include <sys/time.h>
#include <time.h>

/**** **** **** **** Static Prototypes **** **** **** ****/

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                               u16_t value);
static ssize_t read_ct(struct bt_conn *conn,
                       const struct bt_gatt_attr *attr,
		       void * buf,
                       u16_t  len,
                       u16_t  offset);
static ssize_t write_ct(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
			const void *buf,
                        u16_t len,
                        u16_t offset,
			u8_t flags);
static void generate_current_time(u8_t *buf);
static ssize_t read_ct_local_info(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  void * buf,
                                  u16_t  len,
                                  u16_t  offset);
static ssize_t write_ct_local_info(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
			const void *buf,
                        u16_t len,
                        u16_t offset,
                                   u8_t flags);
static ssize_t read_ct_ref_info(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                void * buf,
                                u16_t  len,
                                u16_t  offset);
static ssize_t write_ct_ref_info(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 const void *buf,
                                 u16_t len,
                                 u16_t offset,
                                 u8_t flags);


static u8_t current_time[10];
static u8_t local_time_info[10];
static u8_t ref_time_info[10];
static u8_t ct_update;
struct timeval tv;
u8_t buf[10];

#define BT_UUID_CTS_LOCAL_TIME_INFORMATION BT_UUID_DECLARE_16(0x2a0f)
#define BT_UUID_CTS_REFERENCE_TIME_UPDATE BT_UUID_DECLARE_16(0x1806)

/* Current Time Service Declaration */
BT_GATT_SERVICE_DEFINE(
    cts_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
    BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME,
                           BT_GATT_CHRC_READ   |
                           BT_GATT_CHRC_NOTIFY |
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ   |
                           BT_GATT_PERM_WRITE,
                           read_ct,
                           write_ct,
                           current_time),

    BT_GATT_CCC(ct_ccc_cfg_changed,
                BT_GATT_PERM_READ |
                BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_CTS_LOCAL_TIME_INFORMATION,
                           BT_GATT_CHRC_READ   |
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ   |
                           BT_GATT_PERM_WRITE,
                           read_ct_local_info,
                           write_ct_local_info,
                           local_time_info),

    BT_GATT_CHARACTERISTIC(BT_UUID_CTS_REFERENCE_TIME_UPDATE,
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           read_ct_ref_info,
                           write_ct_ref_info,
                           ref_time_info)
    );

/**** **** **** **** Peripheral API **** **** **** ****/

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

/**** **** **** **** Static Prototypes **** **** **** ****/

/* Obtain current time. */
static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
    /* TODO: Handle value */
}

static ssize_t read_ct(struct bt_conn *conn,
                       const struct bt_gatt_attr *attr,
		       void * buf,
                       u16_t  len,
                       u16_t  offset)
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

static ssize_t read_ct_local_info(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  void * buf,
                                  u16_t  len,
                                  u16_t  offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(local_time_info));
}

static ssize_t write_ct_local_info(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
			const void *buf,
                        u16_t len,
                        u16_t offset,
			u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(local_time_info)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static ssize_t read_ct_ref_info(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                void * buf,
                                u16_t  len,
                                u16_t  offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(ref_time_info));
}

static ssize_t write_ct_ref_info(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 const void *buf,
                                 u16_t len,
                                 u16_t offset,
                                 u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(ref_time_info)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static void generate_current_time(u8_t *buf)
{
    u16_t year;
    struct tm *nowtm;

    /* 'Exact Time 256' contains 'Day Date Time' which contains
     * 'Date Time' - characteristic contains fields for:
     * year, month, day, hours, minutes and seconds.
     */

    int res = gettimeofday(&tv, NULL);

    if (res < 0) {
        printk("Error in gettimeofday(): %d\n", errno);
    }
    else
    {
        nowtm = localtime(&tv.tv_sec);
        printk ("Pphl: %02u:", nowtm->tm_hour);
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
