#ifndef __HIDLINK_BLE_PROTOCOL__
#define __HIDLINK_BLE_PROTOCOL__


typedef enum {
    HIDLINK_PROTOCOL_COMMAND_NONE = 0x00,
    HIDLINK_PROTOCOL_COMMAND_STATUS_GET = 0x01,
    HIDLINK_PROTOCOL_COMMAND_SCAN_START = 0x02,
    HIDLINK_PROTOCOL_COMMAND_SCAN_STOP = 0x03,
    HIDLINK_PROTOCOL_COMMAND_SCAN_COUNT = 0x04,
    HIDLINK_PROTOCOL_COMMAND_SCAN_DATA = 0x05,
    HIDLINK_PROTOCOL_COMMAND_DEVICE_ATTACH = 0x06,
    HIDLINK_PROTOCOL_COMMAND_DEVICE_INFO = 0x07,
    HIDLINK_PROTOCOL_COMMAND_DEVICE_REMOVE = 0x08,
    HIDLINK_PROTOCOL_COMMAND_MAX,
} hidlink_protocol_command_t;


void hidlink_ble_protocol_parser(uint8_t *data, uint32_t len);

#endif