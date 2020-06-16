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

#define BT_UUID_FITNESS_MACHINE BT_UUID_DECLARE_16(0x1826)
#define BT_UUID_TENDED_PROTECT  BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00001891, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_FALL BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00003005, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_NOMOVE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00003007, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_SOS BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x0000300a, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_NOTIF BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x0000300c, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_AUTOCI BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x0000300d, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_TENDED_ID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x0000300e, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_UART_NOTIFY BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00002e29, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_UART_TX BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00002e2a, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BT_UUID_UART_RX BT_UUID_DECLARE_128(BT_UUID_128_ENCODE \
          (0x00002e2b, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))

/**** **** **** **** Static Prototypes **** **** **** ****/

//static u8_t notify_func(struct bt_conn *conn,
//                        struct bt_gatt_subscribe_params *params,
//                        const void *data, u16_t length);
static u8_t discover_cb(struct bt_conn *conn,
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
//static void discover_primary(struct bt_uuid * reqd,
//                             u16_t handle);

static bt_state_t bt_state = BT_DOWN_STATE;

/**** **** **** **** Local Variables **** **** **** ****/

static u8_t g_flag;
static const bt_addr_le_t * g_addr;
static struct bt_conn * default_conn;
//static struct bt_uuid_16 uuid16 = BT_UUID_INIT_16(0);
//static struct bt_uuid_128 uuid128 = BT_UUID_INIT_128(0);
//static bool b_use_128 = false;
static struct bt_gatt_discover_params discover_params = {
    .func = discover_cb,
    .uuid = NULL,
    .start_handle = 0x0001,
    .end_handle = 0xffff
};

//static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_conn_cb conn_callbacks = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
};

typedef struct FLAGS_T
{
    const char * name;
    u8_t length;
    void * callback;
    struct bt_uuid * uuid;
} flag_t;

typedef enum READ_TYPE_T
{
    BATTERY_TYPE,
    SOS_TYPE,
    FALL_TYPE,
    NOMOVE_TYPE,
    NOTIF_TYPE,
    AUTOCI_TYPE,
    ID_TYPE,
    UART_NOTIFY,
    UART_TX_TYPE,
    UART_RX_TYPE,
    READ_TYPE_SIZE
} read_type_t;

K_SEM_DEFINE(wait_sem, 1, 1);

static u8_t read_bat_cb(struct bt_conn *conn,
                        u8_t err,
                        struct bt_gatt_read_params *params,
                        const void *data,
                        u16_t length);

flag_t flags[READ_TYPE_SIZE] = {
    {
        .name = "Battery: ",
        .length = 1,
        .uuid = BT_UUID_BAS_BATTERY_LEVEL
    },
    {
        .name = "SOS:     ",
        .length = 2,
        .uuid = BT_UUID_TENDED_SOS,
    },
    {
        .name = "FALL:    ",
        .length = 2,
        .uuid = BT_UUID_TENDED_FALL
    },
    {
        .name = "NOMOVE:  ",
        .length = 4,
        .uuid = BT_UUID_TENDED_NOMOVE
    },
    {
        .name = "NOTIF:   ",
        .length = 2,
        .uuid = BT_UUID_TENDED_NOTIF
    },
    {
        .name = "AUTO CI: ",
        .length = 2,
        .uuid = BT_UUID_TENDED_AUTOCI
    },
    {
        .name = "ID:      ",
        .length = 4,
        .uuid = BT_UUID_TENDED_ID
    },
    {
        .name = "UART NT: ",
        .length = 2,
        .uuid = BT_UUID_UART_NOTIFY
    },
    {
        .name = "UART TX: ",
        .length = 2,
        .uuid = BT_UUID_UART_TX
    },
    {
        .name = "UART RX: ",
        .length = 19,
        .uuid = BT_UUID_UART_RX
    }
};


/**** **** **** **** BLE API **** **** **** ****/


void print_battery(u8_t level)
{
    printk("Battery: ");
    if(level > 100)
    {
        printk("%d%% (charging)\n",  level - 100);
    }
    else
    {
        printk("%d%%\n",  level);
    }
    k_sem_give(&wait_sem);
}

static u8_t read_bat_cb(struct bt_conn *conn,
                    u8_t err,
                    struct bt_gatt_read_params *params, const void *data,
                    u16_t length)
{
    u8_t * ptr = (u8_t * ) data;

    if(err)
    {
        printk("ERROR: err: %d\n", err);
    }
    else if (!data)
    {
        printk("ERROR: !not data\n");
        (void)memset(params, 0, sizeof(*params));
    }
    else if(length > 1)
    {
        printk("ERROR: Length too long! (%d)\n", length);
    }
    else
    {
        print_battery(ptr[0]);
    }

    return BT_GATT_ITER_STOP;
}

void read_battery(void)
{
    static struct bt_gatt_read_params read_params;

    read_params.func = read_bat_cb;
    read_params.handle_count = 0;
    read_params.by_uuid.start_handle = 0x0001;
    read_params.by_uuid.end_handle = 0xffff;
    read_params.by_uuid.uuid = BT_UUID_BAS_BATTERY_LEVEL;
    if (bt_gatt_read(default_conn, &read_params) < 0) {
        printk("ERROR: read_battery() < 0\n");
    }
}

static u8_t read_flag_cb(struct bt_conn *conn,
                         u8_t err,
                         struct bt_gatt_read_params *params,
                         const void *data,
                         u16_t length)
{
    u8_t * ptr = (u8_t * ) data;
    u8_t size = flags[g_flag].length;

    if(err)
    {
        printk("ERROR: err: %d\n", err);
    }
    else if (!data)
    {
        printk("ERROR: !not data\n");
        (void)memset(params, 0, sizeof(*params));
    }
    else if(length > size)
    {
        printk("ERROR: Length too long! (%d)\n", length);
    }
    else
    {
        printk("%s", flags[g_flag].name);
        for(int count = 0; count < size; count++)
        {
            printk("%02x,", ptr[count]);
        }
        printk(" len: %02d\n", length);
    }

    k_sem_give(&wait_sem);
    return BT_GATT_ITER_STOP;
}

void read_flag(read_type_t type)
{
    static struct bt_gatt_read_params read_params;

    read_params.func = read_flag_cb;
//    read_params.func = flags[(u8_t) type].callback;
    read_params.handle_count = 0;
    read_params.by_uuid.start_handle = 0x0001;
    read_params.by_uuid.end_handle = 0xffff;
    read_params.by_uuid.uuid = flags[type].uuid;
    if (bt_gatt_read(default_conn, &read_params) < 0)
    {
        printk("ERROR: read_flag(%d)\n", type);
    }
}

#define AD_TYPE_BLE_DEVICE_ADDR_TYPE_PUBLIC 0UL
#define AD_TYPE_BLE_DEVICE_ADDR_TYPE_RANDOM 1UL

static u8_t read_bat_cb(struct bt_conn *conn,
                        u8_t err,
                        struct bt_gatt_read_params *params,
                        const void *data,
                        u16_t length);


static void write_flag_cb(struct bt_conn * conn,
                          unsigned char ch,
                          struct bt_gatt_write_params * write_params)
{
    printk("Callback\n");
    printk("ch %d\n",  ch);

    k_sem_give(&wait_sem);
}

//  struct bt_gatt_write_params {
//      /** Response callback */
//      bt_gatt_write_func_t func;
//      /** Attribute handle */
//      u16_t handle;
//      /** Attribute data offset */
//      u16_t offset;
//      /** Data to be written */
//      const void *data;
//      /** Length of the data */
//      u16_t length;
//  };

int write_flag(void)
{
    int ret = 0;
    static u16_t data = 11;
    static struct bt_gatt_write_params write_params;
    write_params.func = write_flag_cb;
    write_params.handle = 0x05;
    write_params.offset = 0;
    write_params.data = &data;
    write_params.length = 2;

    if (k_sem_take(&wait_sem, K_SECONDS(5)) != 0)
    {
        printk("Error! Semiphore in write flag!");
    }
    else
    {
        ret = bt_gatt_write(default_conn, &write_params);
        printk("ret = %d\n", ret);
    }

//    for(;;)
//    {
//        if (k_sem_take(&wait_sem, K_SECONDS(5)) != 0)
//        {
//            printk("Waiting!");
//        }
//        else
//        {
//            break;
//        }
//        k_sleep(K_MSEC(100));
//    }
    return ret;
}

void disconnect(void)
{
    if(k_sem_take(&wait_sem, K_SECONDS(5)) != 0)
    {
        printk("Error! Semiphore in disconnect()");
    }
    else
    {
        k_sleep(K_SECONDS(10));
        int dis = bt_conn_disconnect(default_conn, 0);//BT_HCI_ERR_LOCALHOST_TERM_CONN);
        if(dis != 0)
        {
            printk("ERROR! Failed to disconnect!\n");
        }

        for(;;)
        {
            if(k_sem_take(&wait_sem, K_SECONDS(5)) != 0)
            {
                printk("ERROR! disconnecting\n");
            }
            else
            {
                k_sem_give(&wait_sem);
                break;
            }
        }

    }
}

int read_flags(void)
{
    u8_t count;
    u8_t ret = 1;
    static u8_t count2 = 0;

    for(count = 0; count < READ_TYPE_SIZE; count++)
    {
        for(;;)
        {
            if(count == 0)
            {
                printk("\n");
            }
            if((count2 >= 2) && (bt_state != BT_DISCONNECTED_STATE))
            {
                ret = 0;
                count2 = 0;
//                bt_state = BT_DISCONNECTED_STATE;
                break;
            }
            else if(bt_state != BT_DISCONNECTED_STATE)
            {
                if (k_sem_take(&wait_sem, K_SECONDS(5)) != 0)
                {
                    printk("Error! Semiphore!");
                }
                else
                {
                    if(count == (READ_TYPE_SIZE - 1))
                    {
                        count2++;
                    }
                    g_flag = count;
                    read_flag(g_flag);
                    break;
                }
            }
            else
            {
                //    k_sleep(K_MSEC(100));
                break;
            }
        }
    }
    return ret;
}

int ble_initialise(void)
{
    int ret = bt_enable(NULL);

    if (ret != 0) {
        printk("Bluetooth init failed (ret %d)\n", ret);
    }
    else
    {
        bt_ready();
        bt_conn_cb_register(&conn_callbacks);
        bt_state = BT_IDLE_STATE;
    }
    return ret;
}

bt_state_t get_bt_state(void)
{
    return bt_state;
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

//static bool discovery_match(struct bt_uuid * uuid)
//{
//    bool ret = false;
//
//    if(uuid->type == BT_UUID_TYPE_16)
//    {
//        ret = ( ! bt_uuid_cmp(discover_params.uuid, uuid));
//    }
//    else if(uuid->type == BT_UUID_TYPE_128)
//    {
//        ret = true; //( ! bt_uuid_cmp(discover_params.uuid, uuid));
//    }
//
//    return ret;
//}

static u8_t discover_cb(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
                        struct bt_gatt_discover_params *params)
{
    static struct bt_uuid_128 tended_service_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x00001891, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));
    static struct bt_uuid_16 uuid16 = BT_UUID_INIT_16(0);
    if (attr)
    {

        if (params->uuid == BT_UUID_GAP) {
                printk(" >>>>>>>>>>>>>>>>>>>>>>>>>Pong service discovered\n");
        }

        printk("params->uuid %p\n", params->uuid);
        printk("params->uuid %p\n", &tended_service_uuid.uuid);

        printk("uuid_type %d\n", attr->uuid->type);
        memcpy(&uuid16, attr->uuid, sizeof(uuid16));
        printk("UUID:  0x%04x\n", (BT_UUID_16(&uuid16.uuid)->val));
//        discover_params.uuid = &uuid16.uuid;
        printk("handle:  0x%04x\n", attr->handle);
//        printk("user_data %d\n", (uint8_t) attr->user_data[0]);
        printk("------------------------------------------------------\n");
//        printk("start handle 0x%04x\n", params->start_handle);
//        printk("end handle 0x%04x\n", params->end_handle);
        printk("discover type 0x%04x\n", params->type);
//        printk("uuid type 0x%04x\n", params->uuid->type);

//        uuid16 = BT_UUID_INIT_16(0);
//        memcpy(&uuid16, params->uuid, sizeof(uuid16));
//        printk("UUID:  0x%04x\n", (BT_UUID_16(&uuid16.uuid)->val));
//        discover_params.uuid = &uuid16.uuid;




//    uint8_t count;
//    static uint8_t discovered = 0;

        printk("attr: %p\n", attr);
//        b_use_128 = false;
//        printk("UUID:  0x%04x\n", (BT_UUID_16(&uuid16.uuid)->val));
//        discover_params.uuid = &uuid16.uuid;
//        discover_params.start_handle = params->start_handle + 3;
//        discover_params.type = BT_GATT_DISCOVER_PRIMARY;
//        bt_gatt_discover(default_conn, &discover_params);

//        discover_primary(NULL, attr-> handle);//services[count], attr->handle);
        return BT_GATT_ITER_CONTINUE;
    }
    else
    {
        printk("Discover complete\n");
        return BT_GATT_ITER_STOP;
    }
    return BT_GATT_ITER_STOP;
}


//static u8_t discover_cb2(struct bt_conn *conn,
//                        const struct bt_gatt_attr *attr,
//                        struct bt_gatt_discover_params *params)
//{
//    printk("------------------------------------------------------\n");
//    printk("params->start_handle 0x%04x\n", params->start_handle);
//    printk("params->type 0x%04x\n", params->type);
//    uint8_t count;
//    static uint8_t discovered = 0;
//    struct bt_uuid * services[] =
//        {
//            BT_UUID_GAP,
//            BT_UUID_GATT,
//            BT_UUID_DIS,
//            BT_UUID_BAS,
//            BT_UUID_FITNESS_MACHINE,
//            BT_UUID_TENDED_PROTECT
//        };
//    u8_t services_count  =
//        sizeof(services) /
//        sizeof(services[0]);
//
//    printk("attr: %p\n", attr);
//    if (attr)
//    {
//        for(count = 0; count < services_count; count++)
//        {
//            printk("********************** here %d ***************\n", count);
//            if(discovery_match(services[count]))
//            {
//                printk(">>> MATCH <<<\n");
//                char uuid_str[64];
//                u8_t size = services[count]->type == BT_UUID_TYPE_16 ? 16 : 128 ;
//
//                bt_uuid_to_str(services[count], uuid_str, size);
//                printk("Discovered Pri Service 0x%s\n", uuid_str);
//                discovered++;
//                count++;
//                if(discovered == services_count)
//                {
//                    bt_state = BT_FOUND_PROTECT_STATE;
//                }
//
//                if(count < services_count)
//                {
//                    if(count < services_count)
//                    {
//                        discover_primary(services[count], attr->handle);
//                    }
//                }
//                break;
//            }
//            printk("********************** NO MATCH ***************\n");
//            //else
//                //return BT_GATT_ITER_STOP;
//        }
//    }
//    else
//    {
//        //printk("Discover complete\n");
//
//        (void)memset(params, 0, sizeof(*params));
//        discovered = 0;
//
//        return BT_GATT_ITER_STOP;
//    }
//    return BT_GATT_ITER_STOP;
//}

//static void test_uuid_create(void)
//{
//	u8_t le16[] = { 0x01, 0x00 };
//	u8_t be16[] = { 0x00, 0x01 };
//	union {
//		struct bt_uuid uuid;
//		struct bt_uuid_16 u16;
//		struct bt_uuid_128 u128;
//	} u;
//
//	/* Create UUID from LE 16 bit byte array */
//	zassert_true(bt_uuid_create(&u.uuid, le16, sizeof(le16)),
//		     "Unable create UUID");
//
//	/* Compare UUID 16 bits */
//	zassert_true(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0001)) == 0,
//		     "Test UUIDs don't match");
//
//	/* Compare UUID 128 bits */
//	zassert_true(bt_uuid_cmp(&u.uuid, &le_128.uuid) == 0,
//		     "Test UUIDs don't match");
//
//	/* Compare swapped UUID 16 bits */
//	zassert_false(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0100)) == 0,
//		     "Test UUIDs match");
//
//	/* Create UUID from BE 16 bit byte array */
//	zassert_true(bt_uuid_create(&u.uuid, be16, sizeof(be16)),
//		     "Unable create UUID");
//
//	/* Compare UUID 16 bits */
//	zassert_false(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0001)) == 0,
//		     "Test UUIDs match");
//
//	/* Compare UUID 128 bits */
//	zassert_false(bt_uuid_cmp(&u.uuid, &le_128.uuid) == 0,
//		     "Test UUIDs match");
//
//	/* Compare swapped UUID 16 bits */
//	zassert_true(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0100)) == 0,
//		     "Test UUIDs don't match");
//}
//
//static void discover_primary(struct bt_uuid * reqd,
//                             u16_t handle)
//{
//    int err;
//
//    if(reqd->type == BT_UUID_TYPE_16)
//    {
//        memcpy(&uuid16, reqd, sizeof(uuid16));
//        b_use_128 = false;
//        printk("UUID:  0x%04x\n", (BT_UUID_16(&uuid16.uuid)->val));
//        discover_params.uuid = &uuid16.uuid;
//        discover_params.start_handle = handle;
//        discover_params.type = BT_GATT_DISCOVER_PRIMARY;
//        err = bt_gatt_discover(default_conn, &discover_params);
//        if (err)
//        {
//            printk("Discover failed (err %d)\n", err);
//        }
//    }
//    else
//    {
//        printk("***  NOT BT_UUID_TYPE_16  ***\n");
//        if(reqd->type == BT_UUID_TYPE_128)
//        {
//            printk("Found BT_UUID_TYPE_128\n");
//            uint8_t uuid_128[16];
//            printk("reqd->type %d\n", reqd->type);
//            memcpy(&uuid_128, BT_UUID_128(reqd)->val,
//                   ARRAY_SIZE(BT_UUID_128(reqd)->val));
//            // sys_mem_swap(uuid_128, sizeof(uuid_128));
//            printk("UUID: ");
//            for(int count = 0; count < 16; count++)
//            {
//                printk("0x%02x ", uuid_128[count]);
//            }
//            printk("\n");
//
//            discover_params.uuid = reqd;
//            discover_params.start_handle = handle;
//            discover_params.type = BT_GATT_DISCOVER_PRIMARY;
//            err = bt_gatt_discover(default_conn, &discover_params);
//            if (err)
//            {
//                printk("Discover failed (err %d)\n", err);
//            }
//
//            sys_mem_swap(uuid_128, sizeof(uuid_128));
//            printk("match %d\n",bt_uuid_cmp(uuid_128, BT_UUID_TENDED_PROTECT));
//            printk("match %d\n", (bt_uuid_cmp(&u.uuid, &le_128.uuid)));
//
//        }
//    }
//}

//  static void discover_primary2(struct bt_uuid * reqd,
//                               u16_t handle)
//  {
//      int err;
//
//      if(reqd->type != BT_UUID_TYPE_16)
//      {
//      }
//      else
//      {
//          memcpy(&uuid16, reqd, sizeof(uuid16));
//          b_use_128 = false;
//          //printk("UUID:  %x\n", (BT_UUID_16(&uuid16.uuid)->val));
//          discover_params.uuid = &uuid16.uuid;
//          discover_params.start_handle = handle;
//          discover_params.type = BT_GATT_DISCOVER_PRIMARY;
//          err = bt_gatt_discover(default_conn, &discover_params);
//          if (err)
//          {
//              printk("Discover failed (err %d)\n", err);
//          }
//      }
//  }

static bool eir_found(struct bt_data *data, void *user_data)
{
    bt_addr_le_t *addr = user_data;
    struct bt_le_conn_param *param;
    struct bt_uuid *uuid;
    bool ret = true;
    u16_t u16;
    int err;
    int i;

    //printk("[AD]: %u data_len %u\n", data->type, data->data_len);
    switch (data->type)
    {
        /* Intentional Fallthrough */
    case BT_DATA_UUID16_SOME:
    case BT_DATA_UUID16_ALL:

        if (data->data_len % sizeof(u16_t) != 0U) {
            printk("AD malformed\n");
            return true;
        }

        //printk("---> \n");
        //printk("len: %02x\n", data->data_len);
        for (i = 0; i < data->data_len; i += sizeof(u16_t))
        {
            memcpy(&u16, &data->data[i], sizeof(u16));
            uuid = BT_UUID_DECLARE_16(u16);
            //printk("uuid: %04x\n", (int)BT_UUID_16(uuid)->val);
            uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
            //printk("uuid: %04x\n", (int)BT_UUID_16(uuid)->val);

            err = bt_le_scan_stop();
            if (err) {
                printk("Stop LE scan failed (err %d)\n", err);
                continue;
            }
// #define BT_GAP_INIT_CONN_INT_MIN                0x0018  /* 30 ms    */
// #define BT_GAP_INIT_CONN_INT_MAX                0x0028  /* 50 ms    */

            /* param = BT_LE_CONN_PARAM(0x0008,           */
            /*                          0x0028,           */
            /*                          0, 400); */

            param = BT_LE_CONN_PARAM_DEFAULT;
            err = bt_conn_le_create(addr,
                                    BT_CONN_LE_CREATE_CONN,
                                    param, &default_conn);
            if (err)
            {
                printk("ERROR: Create conn failed (err %d)\n", err);
            }

            return false;
        }
    }

    return ret;
}

static bool is_tended_protect(const bt_addr_le_t *addr,
                              s8_t rssi,
                              u8_t type,
                              struct net_buf_simple *ad)
{
    int count;
    int len = ad->len;
    int data_type;
    //int size = ad->size;
    char name[20];
    bool found = false;

//    static ssize_t read_name(struct bt_conn *conn,
//                             const struct bt_gatt_attr *attr,
//                             void *buf,
//                             u16_t len,
//                             u16_t offset)
//{

    //printk("len: %d\n", len);
    //printk("size: %d\n", size);

    for(count = 0; count < ad->len; )
    {
        //printk("count %d, ", count);
        len = ad->data[count];
        //printk("len ");
        //printk("%d ", len);
        count++;
        data_type = ad->data[count];
        //printk("AD Data Type: %s\n", ad_data_type_str(data_type));
        count++;
        //printk(", data ");
        for(int x = count ; x < (count + len - 1) ; x++)
        {
            //printk("%02x,", ad->data[x]);
        }

        if(data_type == BT_DATA_NAME_SHORTENED ||
           data_type == BT_DATA_NAME_COMPLETE   )
        {
            //printk("  (");
            int y = 0;
            for(int x = count ; x < (count + len - 1) ; x++)
            {
                //printk("%c", ad->data[x]);
                /* Look for Tended */
                name[y] = ad->data[x];
                name[++y] = '\0';
                if(strcmp("Tended Protect", name) == 0)
                {
                    found = true;
                }
            }
            //printk(")\n");
            if(found)
            {
                printk("Found Tended Protect!\n");
                g_addr = addr;
            }
        }

        //printk("\n");

        count += len - 1;
    }
    return found;
}

static void device_found(const bt_addr_le_t *addr,
                         s8_t rssi,
                         u8_t type,
                         struct net_buf_simple *ad)
{
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    //printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
    //       dev, type, ad->len, rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        if(is_tended_protect(addr, rssi, type, ad))
        {
            bt_data_parse(ad, eir_found, (void *)addr);
        }
    }
}

static void connected_cb(struct bt_conn *conn, u8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    if (conn_err) {
        printk("Failed to connect to %s (%u)\n", addr, conn_err);
        bt_conn_unref(default_conn);
        default_conn = NULL;
        start_scan();
        return;
    }

    //printk("Connected: %s\n", addr);

    if (conn == default_conn) {
        bt_gatt_discover(default_conn, &discover_params);

//        discover_primary(BT_UUID_GAP, 0x0001);
    }
}

static void disconnected_cb(struct bt_conn *conn, u8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if(bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr)))
    {
        printk("ERROR: Disconnect failed!\n");
    }

    bt_state = BT_DOWN_STATE;

    printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

    if (default_conn != conn) {
        return;
    }

    k_sem_give(&wait_sem);

    bt_conn_unref(default_conn);
    default_conn = NULL;
    start_scan();
}

void reconnect(void)
{
    struct bt_le_conn_param *param;
//    static struct bt_conn * conn = default_conn;

    printk("Reconnect");
    param = BT_LE_CONN_PARAM_DEFAULT;
    bt_conn_le_create(g_addr,
                      BT_CONN_LE_CREATE_CONN,
                      param,
                      &default_conn);
}

static void bt_ready(void)
{
    cts_init();
}
