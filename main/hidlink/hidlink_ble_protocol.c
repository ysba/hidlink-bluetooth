#include "../main.h"
#include "hidlink_private.h"


static void hidlink_send_response(uint32_t tx_index) {

    uint32_t i;
    uint8_t checksum = 0;

    for (i = 0; i < tx_index; i++) {
        checksum += hidlink.tx.data[i];
    }

    hidlink.tx.data[tx_index++] = (checksum ^ ((uint8_t) 0xff)) + 1;

    hidlink.tx.len = tx_index;
    hidlink.tx.index = 0;

    hidlink_ble_indicate();
}

static void hidlink_ble_ack_response(uint8_t ack_val) {

    hidlink.tx.data[0] = 0x3c;
    hidlink.tx.data[1] = hidlink.rx.data[1];
    hidlink.tx.data[2] = 1;
    hidlink.tx.data[3] = ack_val;

    hidlink_send_response(4);
}


static void hidlink_ble_command_status_get() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_ble_ack_response(hidlink.status);
}


static void hidlink_ble_command_scan_start() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_START);
    hidlink_ble_ack_response(0x06);
}


static void hidlink_ble_command_scan_stop() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_STOP);
    hidlink_ble_ack_response(0x06);
}


static void hidlink_ble_command_scan_get_count() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_ble_ack_response(hidlink.hid_peripheral_list.index);
}


static void hidlink_ble_command_scan_get_info_by_index() {

    uint32_t tx_index = 0;
    char name[HIDLINK_PERIPHERAL_MAX_NAME_LEN];
    uint32_t request_index = hidlink.rx.data[3];

    if ((request_index == 0) || (request_index > hidlink.hid_peripheral_list.index)) {

        // invalid index, return nack
        hidlink_ble_ack_response(0x15);
    }
    else {

        // index in protocol starts in 1.
        // internally, index starts in zero.
        request_index -= 1;

        strncpy(name, hidlink.hid_peripheral_list.device[request_index].name, HIDLINK_PERIPHERAL_MAX_NAME_LEN);

        if (strlen(name) > (HIDLINK_PERIPHERAL_MAX_NAME_LEN))
            name[HIDLINK_PERIPHERAL_MAX_NAME_LEN] = 0;

        hidlink.tx.data[tx_index++] = 0x3c;
        hidlink.tx.data[tx_index++] = HIDLINK_PROTOCOL_COMMAND_SCAN_GET_INFO_BY_INDEX;
        hidlink.tx.data[tx_index++] = 1 + sizeof(esp_bd_addr_t) + strlen(name);
        hidlink.tx.data[tx_index++] = 0x06;                    // confirmation with ack
        hidlink.tx.data[tx_index++] = request_index;           // confirms same index from request


        memcpy(&hidlink.tx.data[tx_index], (uint8_t *) &hidlink.hid_peripheral_list.device[request_index].name, sizeof(esp_bd_addr_t));
        tx_index += sizeof(esp_bd_addr_t);

        memcpy(&hidlink.tx.data[tx_index], (uint8_t *) name, strlen(name));
        tx_index += strlen(name);

        hidlink_send_response(tx_index);
    }
}


// static void hidlink_ble_command_attach_to_peripheral() {

//     uint32_t index;
//     uint8_t ack = 0x15;

//     ESP_LOGI(TAG, "%s", __func__);

//     index = hidlink.rx.data[3];

//     if (hidlink.hid_peripheral_list.index == 0) {
        
//         ESP_LOGW(TAG, "hid peripheral list is empty!");
//     }
//     else if (index > hidlink.hid_peripheral_list.index) {
        
//         ESP_LOGW(TAG, "invalid index!");
//     }
//     else {

//         if (index > 0)
//             index -= 1;
        
//         if (esp_bt_hid_host_connect(hidlink.hid_peripheral_list.device[index].bd_addr) == ESP_OK) {
//             hidlink.status = HIDLINK_STATUS_TRYING_TO_CONNECT;
//             ack = 0x06;
//         }
//     }
    
//     hidlink_ble_ack_response(ack);
// }


// static void hidlink_ble_command_clear_bond_device_list() {

//     ESP_LOGI(TAG, "%s", __func__);
//     hidlink_set_command(HIDLINK_COMMAND_CLEAR_BOND_DEVICE_LIST);
//     hidlink_ble_ack_response(0x06);
// }


void hidlink_ble_protocol_parser(uint8_t *data, uint32_t len) {

    uint8_t rx_data;

    if (hidlink.rx.data == NULL) {
        ESP_LOGE(TAG, "rx buffer invalid pointer!");
    }
    else {

        while (len--) {

            rx_data = *data++;

            if (hidlink.rx.push < HIDLINK_PROTOCOL_BUFFER_LEN) {
                hidlink.rx.data[hidlink.rx.push++] = rx_data;
            }

            switch (hidlink.rx.state) {

                case HIDLINK_PROTOCOL_STATE_HEADER: {
                    if (rx_data == 0x3e) {
                        hidlink.rx.data[0] = rx_data;
                        hidlink.rx.push = 1;
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_COMMAND;
                    }
                    break;
                }

                case HIDLINK_PROTOCOL_STATE_COMMAND: {
                    if ((hidlink_protocol_command_t) rx_data < HIDLINK_PROTOCOL_COMMAND_MAX) {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_LEN;
                    }
                    else {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;
                    }
                    break;
                }

                case HIDLINK_PROTOCOL_STATE_LEN: {
                    if (rx_data == 0) {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_CHECKSUM;
                    }
                    else if (rx_data < (HIDLINK_PROTOCOL_BUFFER_LEN - 3)) {
                        hidlink.rx.data_count = rx_data;
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_DATA;
                    }
                    else {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;
                    }

                    break;
                }

                case HIDLINK_PROTOCOL_STATE_DATA: {
                    if (--hidlink.rx.data_count == 0) {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_CHECKSUM;
                    }
                    break;
                }

                case HIDLINK_PROTOCOL_STATE_CHECKSUM: {

                    uint32_t i;
                    uint8_t checksum = 0;

                    for (i = 0; i < hidlink.rx.push; i++) {
                        checksum += hidlink.rx.data[i];
                    }

                    if (checksum == 0) {

                        switch ((hidlink_protocol_command_t) hidlink.rx.data[1]) {

                            case HIDLINK_PROTOCOL_COMMAND_STATUS_GET: {
                                hidlink_ble_command_status_get();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_START: {
                                hidlink_ble_command_scan_start();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_STOP: {
                                hidlink_ble_command_scan_stop();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_GET_COUNT: {
                                hidlink_ble_command_scan_get_count();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_GET_INFO_BY_INDEX: {
                                hidlink_ble_command_scan_get_info_by_index();
                                break;
                            }

                            // case HIDLINK_PROTOCOL_COMMAND_ATTACH_TO_PERIPHERAL: {
                            //     hidlink_ble_command_attach_to_peripheral();
                            //     break;
                            // }

                            // case HIDLINK_PROTOCOL_COMMAND_CLEAR_BOND_DEVICE_LIST: {
                            //     hidlink_ble_command_clear_bond_device_list();
                            //     break;
                            // }

                            // case HIDLINK_PROTOCOL_COMMAND_SHOW_LAST_ATTACHED_PERIPHEAL: {

                            //     break;
                            // }

                            // case HIDLINK_PROTOCOL_COMMAND_CONNECT_LAST_ATTACHED_PERIPHEAL: {

                            //     break;
                            // }

                            default: {
                                ESP_LOGW(TAG, "invalid command!");
                            }
                        }
                    }

                    hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;

                    break;
                }
            }
        }
    }    
}
