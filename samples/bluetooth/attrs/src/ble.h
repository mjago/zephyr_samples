#ifndef _BLE_H
#define _BLE_H

typedef enum BT_STATE_T
{
    BT_DOWN_STATE,
    BT_IDLE_STATE,
    BT_SCANNING_STATE,
    BT_FOUND_PROTECT_STATE,
    BT_CONNECTED_STATE,
    BT_DISCONNECTED_STATE,
    BT_STATE_SIZE
} bt_state_t;

void gatt_register(void);

#endif /* ! _BLE_H */
