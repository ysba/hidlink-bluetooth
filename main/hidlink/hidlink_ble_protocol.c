#include "../main.h"
#include "hidlink_private.h"

static void hidlink_ble_command_status_get();
static void hidlink_ble_command_scan_start();
static void hidlink_ble_command_scan_stop();
static void hidlink_ble_command_scan_count();
static void hidlink_ble_command_scan_data();
static void hidlink_ble_command_device_attach();
static void hidlink_ble_command_device_info();
static void hidlink_ble_command_device_remove();

static void hidlink_ble_ack_response(uint8_t ack_value);
static void hidlink_ble_prepare_response(hidlink_protocol_command_t command);
static void hidlink_send_response();
static uint8_t hidlink_ble_protocol_checksum(uint8_t *data, uint32_t len);


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

                case HIDLINK_PROTOCOL_STATE_HEADER:
                    if (rx_data == 0x3e) {
                        hidlink.rx.data[0] = rx_data;
                        hidlink.rx.push = 1;
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_COMMAND;
                    }
                    break;

                case HIDLINK_PROTOCOL_STATE_COMMAND:
                    if ((hidlink_protocol_command_t) rx_data < HIDLINK_PROTOCOL_COMMAND_MAX) {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_LEN;
                    }
                    else {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;
                    }
                    break;

                case HIDLINK_PROTOCOL_STATE_LEN:
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

                case HIDLINK_PROTOCOL_STATE_DATA:
                    if (--hidlink.rx.data_count == 0) {
                        hidlink.rx.state = HIDLINK_PROTOCOL_STATE_CHECKSUM;
                    }
                    break;

                case HIDLINK_PROTOCOL_STATE_CHECKSUM:

                    uint32_t i;
                    uint8_t checksum = 0;

                    for (i = 0; i < hidlink.rx.push; i++) {
                        checksum += hidlink.rx.data[i];
                    }

                    if (checksum == 0) {

                        switch ((hidlink_protocol_command_t) hidlink.rx.data[1]) {

                            case HIDLINK_PROTOCOL_COMMAND_STATUS_GET:
                                hidlink_ble_command_status_get();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_START:
                                hidlink_ble_command_scan_start();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_STOP:
                                hidlink_ble_command_scan_stop();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_COUNT:
                                hidlink_ble_command_scan_count();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_SCAN_DATA:
                                hidlink_ble_command_scan_data();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_DEVICE_ATTACH:
                                hidlink_ble_command_device_attach();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_DEVICE_INFO:
                                hidlink_ble_command_device_info();
                                break;

                            case HIDLINK_PROTOCOL_COMMAND_DEVICE_REMOVE:
                                hidlink_ble_command_device_remove();
                                break;

                            default:
                                ESP_LOGW(TAG, "invalid command!");
                        }
                    }

                    hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;

                    break;
            }
        }
    }    
}


void hidlink_ble_command_status_get() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_ble_ack_response(hidlink.status);
}


void hidlink_ble_command_scan_start() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_START);
    hidlink_ble_ack_response(0x06);
}


void hidlink_ble_command_scan_stop() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_STOP);
    hidlink_ble_ack_response(0x06);
}


void hidlink_ble_command_scan_count() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_ble_ack_response(hidlink.hid_peripheral_list.index);
}


void hidlink_ble_command_scan_data() {

    char name[HIDLINK_PERIPHERAL_MAX_NAME_LEN];
    uint32_t request_index = hidlink.rx.data[3];

    if ((request_index == 0) || (request_index > hidlink.hid_peripheral_list.index)) {

        hidlink_ble_ack_response(0x15);
    }
    else {

        request_index -= 1;

        strncpy(name, hidlink.hid_peripheral_list.device[request_index].name, HIDLINK_PERIPHERAL_MAX_NAME_LEN);

        if (strlen(name) > (HIDLINK_PERIPHERAL_MAX_NAME_LEN))
            name[HIDLINK_PERIPHERAL_MAX_NAME_LEN] = 0;

        hidlink_ble_prepare_response(HIDLINK_PROTOCOL_COMMAND_SCAN_DATA);

        hidlink.tx.data[hidlink.tx.len++] = 1 + sizeof(esp_bd_addr_t) + strlen(name);
        hidlink.tx.data[hidlink.tx.len++] = 0x06;
        hidlink.tx.data[hidlink.tx.len++] = request_index;

        memcpy(&hidlink.tx.data[hidlink.tx.len], (uint8_t *) &hidlink.hid_peripheral_list.device[request_index].name, sizeof(esp_bd_addr_t));
        hidlink.tx.len += sizeof(esp_bd_addr_t);

        memcpy(&hidlink.tx.data[hidlink.tx.len], (uint8_t *) name, strlen(name));
        hidlink.tx.len += strlen(name);

        hidlink_send_response();
    }
}


void hidlink_ble_command_device_attach() {

    uint32_t index;
    uint8_t ack = 0x15;

    ESP_LOGI(TAG, "%s", __func__);

    index = hidlink.rx.data[3];

    if (hidlink.hid_peripheral_list.index == 0) {
        
        ESP_LOGW(TAG, "hid peripheral list is empty!");
    }
    else if (index > hidlink.hid_peripheral_list.index) {
        
        ESP_LOGW(TAG, "invalid index!");
    }
    else {

        if (index > 0)
            index -= 1;
        
        if (esp_bt_hid_host_connect(hidlink.hid_peripheral_list.device[index].bd_addr) == ESP_OK) {
            hidlink.status = HIDLINK_STATUS_TRYING_TO_CONNECT;
            ack = 0x06;
        }
    }
    
    hidlink_ble_ack_response(ack);
}


void hidlink_ble_command_device_info() {
    ESP_LOGI(TAG, "%s", __func__);
    // #TODO: implement this command
    hidlink_ble_ack_response(0x06);
}


void hidlink_ble_command_device_remove() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_CLEAR_BOND_DEVICE_LIST);
    hidlink_ble_ack_response(0x06);
}


void hidlink_ble_ack_response(uint8_t ack_value){

    hidlink.tx.len = 0;
    hidlink.tx.data[hidlink.tx.len++] = 0x3c;
    hidlink.tx.data[hidlink.tx.len++] = hidlink.rx.data[1];
    hidlink.tx.data[hidlink.tx.len++] = 1;
    hidlink.tx.data[hidlink.tx.len++] = ack_value;
    hidlink_send_response();
}


void hidlink_ble_prepare_response(hidlink_protocol_command_t command) {
    hidlink.tx.len = 0;
    hidlink.tx.data[hidlink.tx.len++] = 0x3c;
    hidlink.tx.data[hidlink.tx.len++] = command;
}

void hidlink_send_response() {

    uint8_t checksum;

    checksum = hidlink_ble_protocol_checksum(hidlink.tx.data, hidlink.tx.len);
    hidlink.tx.data[hidlink.tx.len++] = checksum;
    hidlink.tx.index = 0;
    hidlink_ble_indicate();
}


uint8_t hidlink_ble_protocol_checksum(uint8_t *data, uint32_t len) {

    uint8_t checksum = 0;

    while (len--) {
        checksum += *data++;
    }

    return (~checksum + 1);
}
