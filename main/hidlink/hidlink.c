/**
 * @file hidlink.c
 * @brief External hidlink interface functions.
 *
 * These functions needs to be called externally for proper hidlink operation.
 */


#include "../main.h"
#include "hidlink_private.h"


// bluetooth device name (same for both ble and classic)
char *dev_name = "hidlink";


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
    
    uint32_t i;
    uint32_t name_len;
    char bda_str[18];

    if (hidlink.hid_peripheral_list.index >= HIDLINK_DEVICES_NUMBER) {

        ESP_LOGW(TAG, "%s, not adding peripheral to list: %s [%s]. list is full with %lu devices",
                __func__,
                name,
                bda2str(*bd_addr, bda_str, 18),
                (uint32_t) HIDLINK_DEVICES_NUMBER
            );

    }
    else {
        // check if device is already on list
        // in case "i" value reaches "index" value, address was not found in list
        for (i = 0; i < hidlink.hid_peripheral_list.index; i++) {

            if (!memcmp(&hidlink.hid_peripheral_list.device[i].bd_addr, (uint8_t *) bd_addr, 6)) {
                break;
            }
        }

        if (i < hidlink.hid_peripheral_list.index) {

            // device already on the list

            ESP_LOGD(TAG, "%s, peripheral already on list: %s [%s]",
                __func__,
                name,
                bda2str(*bd_addr, bda_str, 18)
            );
        }
        else {

            // add new device to list
            name_len = strlen(name);

            if (name_len > (HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1))
                name_len = HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1;

            ESP_LOGI(TAG, "%s, adding peripheral to list: %s [%s]",
                __func__,
                name,
                bda2str(*bd_addr, bda_str, 18)
            );

            memcpy(&hidlink.hid_peripheral_list.device[i].bd_addr, bd_addr, sizeof(esp_bd_addr_t));
            memset(&hidlink.hid_peripheral_list.device[i].name, 0, ESP_BT_GAP_MAX_BDNAME_LEN);
            memcpy(&hidlink.hid_peripheral_list.device[i].name, name, name_len);

            // sends indication to the BLE interface
            // #TODO: REMOVE? NOW IS SENT VIA REQUEST FROM BLE
            //hidlink_send_hid_peripheral_data(hidlink.hid_peripheral_list.index, bd_addr, name);
            
            hidlink.hid_peripheral_list.index++; 
        }
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
 * @brief Get hidlink current status
 *
 * This function is called to get current hidlink status (ie. connected, idle, etc).
 */
hidlink_status_t hidlink_get_status() {

    return (hidlink.status);
}


void hidlink_set_attached_device_bda(esp_bd_addr_t *bda) {

    memcpy(&hidlink.attached_device.bd_addr, bda, sizeof(esp_bd_addr_t));
}


void hidlink_set_attached_device_name(char *new_name) {

    uint32_t len = strlen(new_name);

    if (len > (HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1))
        len = HIDLINK_PERIPHERAL_MAX_NAME_LEN - 1;

    strncpy(hidlink.attached_device.name, new_name, len);
}
