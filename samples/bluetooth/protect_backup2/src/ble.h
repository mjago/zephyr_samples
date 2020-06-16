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

void ble_initialise(void);
bt_state_t get_bt_state(void);
void start_scan(void);
void protect_connect(void);
void read_battery(void);
void read_sos(void);
void read_fall(void);
void read_nomove(void);
void read_notif(void);
void read_flags(void);
void disconnect(void);
void reconnect(void);

#endif /* ! _BLE_H */
