#ifndef __HIDLINK__
#define __HIDLINK__


#include "hidlink_ble_protocol.h"


typedef enum {
    HIDLINK_STATE_API_INIT = 0,
    HIDLINK_STATE_API_DEINIT,
	HIDLINK_STATE_IDLE,
} hidlink_state_t;


typedef enum {
    HIDLINK_COMMAND_NONE = 0,
    HIDLINK_COMMAND_SCAN_START,
    HIDLINK_COMMAND_SCAN_STOP,
    HIDLINK_COMMAND_SCAN_DONE,
    HIDLINK_COMMAND_SET_STATUS_CONNECTED,
    HIDLINK_COMMAND_SET_STATUS_IDLE,
    HIDLINK_COMMAND_CLEAR_BOND_DEVICE_LIST,
    HIDLINK_COMMAND_SHOW_ATTACHED_DEVICE_INFO,
} hidlink_command_t;


typedef enum {
    HIDLINK_STATUS_NONE = 0,
    HIDLINK_STATUS_NOT_CONNECTED,
    HIDLINK_STATUS_SCANNING,
    HIDLINK_STATUS_TRYING_TO_CONNECT,
    HIDLINK_STATUS_CONNECTED,
} hidlink_status_t; 


typedef enum {
    HIDLINK_PROTOCOL_STATE_HEADER = 0,
    HIDLINK_PROTOCOL_STATE_COMMAND,
    HIDLINK_PROTOCOL_STATE_LEN,
    HIDLINK_PROTOCOL_STATE_DATA,
    HIDLINK_PROTOCOL_STATE_CHECKSUM,
} hidlink_protocol_state_t;


void hidlink_start();
void hidlink_set_command(hidlink_command_t command);
void hidlink_add_hid_peripheral(esp_bd_addr_t *bd_addr, char *name);
void hidlink_ble_set_char_handle(uint16_t char_handle);
void hidlink_ble_set_mtu(uint16_t mtu);
void hidlink_set_ble_connection_info(esp_gatt_if_t gatts_if, uint16_t conn_id, esp_bd_addr_t *bda);
void hidlink_set_cccd(uint16_t val);
hidlink_status_t hidlink_get_status();
void hidlink_set_attached_device_bda(esp_bd_addr_t *bda);
void hidlink_set_attached_device_name(char *new_name);

char *hidlink_get_device_name();

extern char *dev_name;

#endif
