#include "../main.h"
#include "hidlink_private.h"


static void hidlink_ble_ack_response(uint8_t ack_val) {

    uint8_t checksum = 0;
    uint32_t i;

    hidlink.tx.data[0] = 0x3c;
    hidlink.tx.data[1] = hidlink.rx.data[1];
    hidlink.tx.data[2] = 1;
    hidlink.tx.data[3] = ack_val;

    for (i = 0; i < 4; i++) {
        checksum += hidlink.tx.data[i];
    }

    hidlink.tx.data[4] = (checksum ^ ((uint8_t) 0xff)) + 1;

    hidlink.tx.len = 5;
    hidlink.tx.index = 0;

    hidlink_ble_indicate();
}


static void hidlink_ble_command_get_status() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_ble_ack_response(hidlink.status);
}


static void hidlink_ble_command_start_scan() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_START);
    hidlink_ble_ack_response(0x06);
}


static void hidlink_ble_command_stop_scan() {

    ESP_LOGI(TAG, "%s", __func__);
    hidlink_set_command(HIDLINK_COMMAND_SCAN_STOP);
    hidlink_ble_ack_response(0x06);
}


static void hidlink_ble_command_attach_to_peripheral() {

    uint32_t index;
    uint8_t ack = 0x15;

    ESP_LOGI(TAG, "%s", __func__);

    index = hidlink.rx.data[3];
    
    if (hidlink_attach_to_device(index) == true) {
        hidlink_ble_ack_response(0x06);
    }
    
    hidlink_ble_ack_response(ack);
}


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

                            case HIDLINK_PROTOCOL_COMMAND_GET_STATUS: {
                                hidlink_ble_command_get_status();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_START_SCAN: {
                                hidlink_ble_command_start_scan();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_STOP_SCAN: {
                                hidlink_ble_command_stop_scan();
                                break;
                            }

                            case HIDLINK_PROTOCOL_COMMAND_ATTACH_TO_PERIPHERAL: {
                                hidlink_ble_command_attach_to_peripheral();
                                break;
                            }

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
