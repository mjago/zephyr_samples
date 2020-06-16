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

/**** **** **** **** Central API **** **** **** ****/

enum {
    INIT_STATE,
    SCANNING_STATE,
    CONNECTED_STATE,
    READ_FLAGS_STATE,
    WRITE_FLAG_STATE,
    READ_FLAGS_2_STATE,
    DISCONNECT_STATE,
    IDLE,
    IDLE2,
    RESCAN,
    RECONNECT,
    FAIL_STATE,
    FINISH_STATE
} state_t;

static int state = INIT_STATE;

static int  init_state(void);
static int  scanning_state(void);
static void conclude_msg(const char * msg);
static void fail_state(void);
static void finish_state(void);

void main(void)
{
    int ret;
    int count = 0;

    for(;;)
    {
        switch(state)
        {
        case INIT_STATE:
            state = (init_state() == 0) ? SCANNING_STATE : FAIL_STATE;
            break;

        case SCANNING_STATE:
            ret = scanning_state();
            if(ret == 0)
            {
                state = CONNECTED_STATE;
            }
            break;

        case CONNECTED_STATE:
            count = 0;
            state = READ_FLAGS_STATE;
            break;

        case READ_FLAGS_STATE:
            ret = read_flags();
            k_sleep(K_MSEC(100));
            if(ret == 0)
            {
                state = WRITE_FLAG_STATE;
            }
        break;

        case WRITE_FLAG_STATE:
            ret = write_flag();
            if(ret == 0)
            {
                count = 0;
                state = READ_FLAGS_2_STATE;
            }
            break;

        case READ_FLAGS_2_STATE:
            ret = read_flags();
            k_sleep(K_MSEC(1000));
            if(ret == 0)
            {
                printk("Calling disconnect\n");
                disconnect();
                state = FINISH_STATE;
            }
            break;

        case DISCONNECT_STATE:
            count = 0;
            state = FAIL_STATE;
            break;

        case IDLE:
            k_sleep(K_SECONDS(3));
            start_scan();
            state = RESCAN;
            break;

        case IDLE2:
            start_scan();
            state = RECONNECT;
            break;

        case RESCAN:
            if(get_bt_state() == BT_FOUND_PROTECT_STATE)
            {
                printk("\nConnected\n");
                state = CONNECTED_STATE;
                break;
            }
            break;

        case RECONNECT:
            reconnect();
            state = CONNECTED_STATE;
            break;

        case FAIL_STATE:
            fail_state();
            break;

        case FINISH_STATE:
            finish_state();
            break;

        default: printk("ERROR: Invalid Main State!");
            break;
        }
    }
}

static int init_state(void)
{
    int ret = 1;
    printk("Initialising BLE:");
    if(ble_initialise() != 0)
    {
        printk(" FAILED!\n");
    }
    else
    {
        start_scan();
        printk("\nScanning:\n");
        k_sleep(100);
        ret = 0;
    }
    return ret;
}

static int scanning_state(void)
{
    char c;
    int ret = 1;
    int err;

    k_sleep(1000);
    if(get_bt_state() == BT_FOUND_PROTECT_STATE)
    {
        printk("Found Protect!\n\n");
        printf("Hit SPACEBAR Return if correct TENDED PROTECT.\n");
        printf("Otherwise Just Hit Return:\n");
        err = read(STDIN_FILENO, &c, 1);
        printk("ret = %d\n", c);
        printk("c = %d\n", c);
        if((err == 1) && (c == 32))
        {
            printk("\nConnected\n");
            ret = 0;
        }
    }
    return ret;
}

static void conclude_msg(const char * msg)
{
    static u8_t count = 0;
    const char * pre_str[7] = {"    ", "   .", "  ..", " ...", "....", " ...", "  .."};
    const char * suf_str[7] = {"    ", ".   ", "..  ", "... ", "....", "... ", "..  "};

    fflush(0);
    printk("  %s %s %s\r", pre_str[count], msg, suf_str[count]);
    count = (++count > 6) ? 0 : count;
    k_sleep(K_MSEC(200));
}

static void fail_state(void)
{
    conclude_msg("Given up");
}

static void finish_state(void)
{
    conclude_msg("Finished");
}
