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

#define INIT         0
#define INIT2        1
#define SCAN         2
#define CONNECTED    3
#define READ_FLAGS   4
#define DISCONNECTED 5
#define IDLE         6
#define IDLE2        7
#define RESCAN       8
#define RECONNECT    9

int state = INIT;

void main(void)
{
    int ret;
    char buf[10];
    int count = 0;

    for(;;)
    {
        switch(state)
        {
        case INIT:

            for(;;)
            {
                //    printf("Hit return to GO\n");
//                ret = read(STDIN_FILENO, buf, 1);
//                buf[1] = '\0';
//                printf("ret: %d\n", ret);
//                printf("buf(int): %d\n", buf[0]);

//                printf("line: %s\n", buf);
//                if((buf[0] == 81) || (buf[0] == 113))
//                if(buf[0] == 'q' || buf[0] == 'Q')
//                {
                state = INIT2;
                break;
//            }
            }
            break;

        case INIT2:

            if(get_bt_state() == BT_DOWN_STATE)
            {
                printk("Initialising BLE!\n");
                ble_initialise();
            }
            else if(get_bt_state() == BT_IDLE_STATE)
            {
                state = SCAN;
                start_scan();
                printk("Scanning!\n");
            }
            k_sleep(100);
            break;

        case SCAN:
            k_sleep(1000);
            if(get_bt_state() == BT_FOUND_PROTECT_STATE)
            {
                printk("Found Protect!\n");

                for(;;)
                {
                    printf("Hit SPACEBAR Return if correct TENDED PROTECT.\n");
                    printf("    Otherwise Just Hit Return:\n");
                    ret = read(STDIN_FILENO, buf, 1);
                    buf[1] = '\0';
                    printf("ret: %d\n", ret);
                    printf("buf(int): %d\n", buf[0]);

//                printf("line: %s\n", buffer);
//                    if(buf[0] == 32)
//                if(buffer[0] == 'q' || buffer[0] == 'Q')
//                    {
                    printk("\nConnected\n");
                    state = CONNECTED;
                    break;
//                    }
                }
                break;

//                protect_connect();
            }
            k_sleep(100);
            break;

        case CONNECTED:
            count++;
//            k_sleep(K_SECONDS(5));
//            k_sleep(K_SECONDS(5));
            state = READ_FLAGS;
            break;

        case READ_FLAGS:
            if(get_bt_state() == BT_DISCONNECTED_STATE)
            {
                printk("Disconnected!\n");
                state = DISCONNECTED;
            }
            else
            {
                read_flags();
                state = CONNECTED;
            }
            break;

        case DISCONNECTED:
            count = 0;
//            disconnect();
            state = IDLE;
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
                state = CONNECTED;
                break;
            }
            break;

        case RECONNECT:
            reconnect();
            state = CONNECTED;
            break;

        default: printk("ERROR: Invalid Main State!");
            break;
        }
    }
}
