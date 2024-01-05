/**
 * @file hidlink.c
 * @brief External hidlink interface functions.
 *
 * These functions needs to be called externally for proper hidlink operation.
 */


#include "../main.h"
#include "hidlink_private.h"


/**
 * @brief Start hidlink task.
 *
 * Call this function once at code initialization in app_main().
 * The FreeRTOS hidlink_core_task() is initiated and runs forever.
 */
void hidlink_start() {

    xTaskCreate(hidlink_core_task, "hidlink", 4096, NULL, 10, NULL);
}


/**
 * @brief Set hidlink operation command.
 *
 * Call this function whenever is needed to change hidlink operation mode.
 *
 * @param command Command type from \link hidlink_command_t \endlink enumerator.
 */
void hidlink_set_command(hidlink_command_t command) {
    
    xQueueSend(hidlink.command_queue, &command, portMAX_DELAY);
}


/**
 * @brief Add HID peripheral to list.
 *
 * This function is called whenever a valid HID peripheral is found during Bluetooth scan.
 *
 * When a HID peripheral is added to list, a indication is sent to the BLE interface.
 *
 * @param bd_addr Pointer to discovered device address.
 * @param name Pointer to discovered device name.
 */
void hidlink_add_hid_peripheral(esp_bd_addr_t *bd_addr, char *name) {


    // #TODO: this function needs to filter already added devices
    
    uint32_t i;
    uint32_t name_len;

    i = hidlink.hid_peripheral_list.index;

    name_len = strlen(name);

    if (name_len > (HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1))
        name_len = HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1;

    if (i < HIDLINK_DEVICES_NUMBER) {

        memcpy(&hidlink.hid_peripheral_list.bd_addr[i], bd_addr, sizeof(esp_bd_addr_t));
        memset(&hidlink.hid_peripheral_list.name[i], 0, ESP_BT_GAP_MAX_BDNAME_LEN);
        memcpy(&hidlink.hid_peripheral_list.name[i], name, name_len);

        // sends indication to the BLE interface
        hidlink_send_hid_peripheral_data(hidlink.hid_peripheral_list.index, bd_addr, name);
        
        hidlink.hid_peripheral_list.index++;
    }
}


/**
 * @brief Set BLE characteristic handle
 *
 * This function is called by the BLE stack. 
 * It stores a handle when data is written to a BLE characteristic.
 * The handle is needed by hidlink_ble_indicate() to respond to the same characteristic.
 *
 * @param char_handle BLE characteristic handle.
 */
void hidlink_ble_set_char_handle(uint16_t char_handle) {

    hidlink.ble.chr_handle = char_handle;
}


/**
 * @brief Set BLE MTU
 *
 * This function is called by the BLE stack when there is event to change MTU.
 *
 * @param mtu New MTU value.
 */
void hidlink_ble_set_mtu(uint16_t mtu) {

    hidlink.ble.mtu_size = mtu;
}


/**
 * @brief Set BLE connection information
 *
 * This function is called by the BLE stack at the connection event.
 * It stores information about the connection
 *
 * @param gatts_if GATTS interface handle.
 * @param conn_id Connection identifier.
 * @param bda Remote Bluetooth Device Address.
 */
void hidlink_set_ble_connection_info(esp_gatt_if_t gatts_if, uint16_t conn_id, esp_bd_addr_t *bda) {

    hidlink.ble.conn_id = conn_id;
    hidlink.ble.gatts_if = gatts_if;
    memcpy(&hidlink.ble.remote_bda, bda, sizeof(esp_bd_addr_t));
}


/**
 * @brief Set BLE CCCD
 *
 * This function is called by the BLE stack and it is used to change the CCCD (Client Characteristic Configuration Descriptor).
 * Notifications and/or indications for the BLE characteristics can be enabled or disabled.
 *
 * @param val Type of configuration (1: notify enable, 2: indicate enable, other values: disable both, characteristic is available only through direct reading).
 */
void hidlink_set_cccd(uint16_t val) {

    if(val == 1) {
        ESP_LOGW(TAG, "%s, notify enabled", __func__);
        hidlink.ble.flags.bits.notify_enable = 1;
        hidlink.ble.flags.bits.indicate_enable = 0;
    }
    else if(val == 2) {
        ESP_LOGW(TAG, "%s, indicate enabled", __func__);
        hidlink.ble.flags.bits.notify_enable = 0;
        hidlink.ble.flags.bits.indicate_enable = 1;
    }
    else {
        ESP_LOGW(TAG, "%s, notify and indicate disabled", __func__);
        hidlink.ble.flags.bits.notify_enable = 0;
        hidlink.ble.flags.bits.indicate_enable = 0;
    }
}


/**
 * @brief Get hidlink Bluetooth device name
 *
 * This function gets the hidlink device name to appear in Bluetooth interfaces (BLE and Classic).
 *
 * @return Pointer to name string
 */
char *hidlink_get_device_name() {
    return (dev_name);
}


/**
 * @brief Set hidlink Bluetooth device name
 *
 * This function sets the hidlink device name to appear in Bluetooth interfaces (BLE and Classic).
 *
 * @param Pointer to name string
 */
void hidlink_set_device_name(char *new_name) {
    
    // #TODO: set bluetooth device name
}


/**
 * @brief New HID report
 *
 * This function is called by the Bluetooth stack whenever a HID report is received. 
 * The HID report is then sent to the USB interface via UART.
 *
 * @param data HID report data
 * @param len HID report length
 */
void hidlink_send_hid_report_to_uart(uint8_t *data, uint32_t len) {

    // #TODO: send data to rtos task instead of directly to uart.

    uint8_t buf[32] = {0};
    uint32_t tx_count = 0;
    uint32_t i;
    uint8_t checksum = 0;

    // 0xaa
    // len (max = sizeof(buf) - 3)
    // data[n]
    // checksum

    if (len > (sizeof(buf) - 3)) {
        ESP_LOGW(TAG, "%s, invalid len", __func__);
        return;
    }

    buf[tx_count++] = 0xaa;
    buf[tx_count++] = len;
    memcpy(&buf[2], data, len);
    tx_count += len;

    for (i = 0; i < tx_count; i++)
        checksum += buf[i];

    buf[tx_count++] = ~checksum + 1;

    uart_write_bytes(HIDLINK_UART_PORT_NUM, buf, tx_count);

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, tx_count, ESP_LOG_INFO);
}
