/**
 * @file hidlink_private.h
 * @brief Header containing definitions for hidlink internal operation.
 */

#ifndef __HIDLINK_PRIVATE__
#define __HIDLINK_PRIVATE__


// tag for esp32 log messages
#define TAG          "HIDLINK"

#define HIDLINK_DEVICES_NUMBER              16

#define HIDLINK_PROTOCOL_BUFFER_LEN         64

#define HIDLINK_PERIPHERAL_MAX_NAME_LEN     32

#define HIDLINK_UART_PORT_NUM       2
#define HIDLINK_UART_BUF_SIZE       256


// hidlink_t data structure
typedef struct {
    hidlink_state_t state;
    hidlink_status_t status;
    QueueHandle_t command_queue;
    
    struct {
        uint32_t index;
        esp_bd_addr_t bd_addr[HIDLINK_DEVICES_NUMBER];
        char name[HIDLINK_DEVICES_NUMBER][HIDLINK_PERIPHERAL_MAX_NAME_LEN];
    } hid_peripheral_list;

    struct {
        hidlink_protocol_state_t state;
        uint8_t *data;
        uint32_t push;
        uint32_t data_count;
    } rx;

    struct {
        uint32_t len;
        uint32_t index;
        uint32_t task;
        uint8_t *data;
    } tx;

    struct {
        esp_gatt_if_t gatts_if;
        uint16_t conn_id;
        uint16_t chr_handle;
        esp_bd_addr_t remote_bda;
        uint16_t mtu_size;
        union {
            uint32_t val;
            struct {
                uint32_t notify_enable:1;
                uint32_t indicate_enable:1;
            } bits;
        } flags;
    } ble;
    
    union {
        uint32_t val;
        struct {
            bool scan_start:1;
        } bits;
    } flags;

} hidlink_t;


extern hidlink_t hidlink;


void hidlink_init();
void hidlink_clear_hid_peripheral_list();
void hidlink_ble_indicate();
void hidlink_core_task();
void hidlink_send_hid_peripheral_data(uint8_t peripheral_index, esp_bd_addr_t *bd_addr, char *name);

#endif