/**
 * @file hidlink_private.c
 * @brief Internal hidlink core functions.
 *
 * These functions are parte of hidlink core operation and cannot be called from external scope.
 */

#include "../main.h"
#include "hidlink_private.h"


// instance of hidlink_t structure (only visible to hidlink source files)
hidlink_t hidlink;


// string for loggind bluetooth addresses
static char bda_str[18];

/**
 * @brief Internal function to initialize hidlink data structure.
 */
void hidlink_init() {

    memset(&hidlink, 0, sizeof(typeof(hidlink)));
    
    hidlink.command_queue = xQueueCreate(10, sizeof(hidlink_command_t));

    if (hidlink.command_queue == NULL) {
        ESP_LOGE(TAG, "%s, queue creation failed", __func__);
    }

    hidlink.rx.state = HIDLINK_PROTOCOL_STATE_HEADER;
    hidlink.rx.data = NULL;
    hidlink.rx.push = 0;
    hidlink.rx.data = (uint8_t *) pvPortMalloc(HIDLINK_PROTOCOL_BUFFER_LEN * sizeof(uint8_t));
    if (hidlink.rx.data == NULL) {
        ESP_LOGE(TAG, "rx data malloc error!");
    }
    hidlink.tx.data = (uint8_t *) pvPortMalloc(HIDLINK_PROTOCOL_BUFFER_LEN * sizeof(uint8_t));
    if (hidlink.tx.data == NULL) {
        ESP_LOGE(TAG, "tx data malloc error!");
    }

    hidlink.status = HIDLINK_STATUS_NOT_CONNECTED;
    hidlink.state = HIDLINK_STATE_API_INIT;
}


void hidlink_clear_hid_peripheral_list() {

    ESP_LOGD(TAG, "%s, clearing %d bytes", __func__, sizeof(hidlink.hid_peripheral_list));
    memset(&hidlink.hid_peripheral_list, 0, sizeof(hidlink.hid_peripheral_list));
}


/**
 * @brief teste
 */
void hidlink_ble_indicate() {

    uint32_t byte_to_send;

    if(hidlink.tx.len != 0) {

        // still got data to send

        if(hidlink.tx.len <= (hidlink.ble.mtu_size - 8)) {

            // if amount of data to send is less than mtu, send everything

            byte_to_send = hidlink.tx.len;
        }
        else {

            // if amout of data to send is greater than mtu, send mtu

            byte_to_send = (hidlink.ble.mtu_size - 8);
        }

        hidlink.tx.len -= byte_to_send;

        if(hidlink.ble.flags.bits.notify_enable || hidlink.ble.flags.bits.indicate_enable) {
            
            esp_ble_gatts_send_indicate(
                hidlink.ble.gatts_if,
                hidlink.ble.conn_id,
                hidlink.ble.chr_handle,
                byte_to_send,
                &hidlink.tx.data[hidlink.tx.index],
                true);

            ESP_LOG_BUFFER_HEX_LEVEL(TAG, &hidlink.tx.data[hidlink.tx.index], byte_to_send, ESP_LOG_DEBUG);

            hidlink.tx.index += byte_to_send;

            ESP_LOGV(TAG, "%s, %lu sent, %lu remaining", 
                __func__, 
                byte_to_send, 
                hidlink.tx.len);
        }
        // #TODO: enable write without indication or notify
        else {

        //     esp_ble_gatts_send_response();

            ESP_LOGD(TAG, "%s, indicate and notify off", __func__);

            hidlink.tx.len = 0;
        }
    }
}


/**
 * @brief Core hidlink task.
 *
 * This function is the core hidlink task. It is initialized at app_main() and must run forever.
 *
 */
void hidlink_core_task() {

    esp_err_t err;

    hidlink_init();

    while (1) {

        switch(hidlink.state) {   
            
            case HIDLINK_STATE_API_INIT: {

                ESP_LOGD(TAG, "%s, HIDLINK_STATE_API_INIT, start", __func__);

                uart_config_t uart_config = {
                    .baud_rate = 115200,
                    .data_bits = UART_DATA_8_BITS,
                    .parity = UART_PARITY_DISABLE,
                    .stop_bits = UART_STOP_BITS_1,
                    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                    .source_clk = UART_SCLK_DEFAULT,
                };
                int intr_alloc_flags = ESP_INTR_FLAG_IRAM;


                err = nvs_flash_init();
                if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                    ESP_LOGW(TAG, "%s, nvs_flash_init failed, err %s", __func__, esp_err_to_name(err));
                    nvs_flash_erase();
                    err = nvs_flash_init();    
                }
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "nvs_flash_init failed:  %s", esp_err_to_name(err));
                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }
                else if (bt_classic_init() == false) {

                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }
                else if (bt_low_energy_init() == false) {

                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }

                // #TODO: add serial_port group
                else if((err = uart_driver_install(HIDLINK_UART_PORT_NUM, 
                        HIDLINK_UART_BUF_SIZE, 0, 0, NULL, intr_alloc_flags)) != ESP_OK) {
                    ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }
                else if((err = uart_param_config(HIDLINK_UART_PORT_NUM, &uart_config)) != ESP_OK) {
                    ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }
                else if((err = uart_set_pin(HIDLINK_UART_PORT_NUM, 
                        IO_UART_TX, 
                        UART_PIN_NO_CHANGE, 
                        UART_PIN_NO_CHANGE, 
                        UART_PIN_NO_CHANGE)) != ESP_OK) {
                    ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
                    hidlink.state = HIDLINK_STATE_API_DEINIT;
                }
                else {
                    // api init success
                    ESP_LOGD(TAG, "%s, HIDLINK_STATE_API_INIT, ok", __func__);
                    led_set_blink(LED_HANDLE_GREEN, 1);
                    hidlink.state = HIDLINK_STATE_IDLE;
                }
                break;
            }        

            case HIDLINK_STATE_API_DEINIT: {

                ESP_LOGD(TAG, "%s, HIDLINK_STATE_API_DEINIT, start", __func__);

                led_set_off(LED_HANDLE_GREEN);
                led_set_off(LED_HANDLE_YELLOW);
                led_set_on(LED_HANDLE_RED);
                
                if ((err = esp_bluedroid_disable()) != ESP_OK) {
                    ESP_LOGW(TAG, "%s, esp_bluedroid_disable failed: %s", __func__, esp_err_to_name(err));
                }

                if ((err = esp_bluedroid_deinit()) != ESP_OK) {
                    ESP_LOGW(TAG, "%s, esp_bluedroid_deinit failed: %s", __func__, esp_err_to_name(err));
                }

                if ((err = esp_bt_controller_disable()) != ESP_OK) {
                    ESP_LOGW(TAG, "%s, esp_bt_controller_disable failed: %s", __func__, esp_err_to_name(err));
                }

                if ((err = esp_bt_controller_deinit()) != ESP_OK) {
                    ESP_LOGW(TAG, "%s, esp_bt_controller_deinit failed: %s", __func__, esp_err_to_name(err));
                }

                if ((err = nvs_flash_deinit()) != ESP_OK) {
                    ESP_LOGW(TAG, "%s, nvs_flash_deinit failed: %s", __func__, esp_err_to_name(err));
                }

                ESP_LOGD(TAG, "%s, HIDLINK_STATE_API_DEINIT, 5s delay then back to HIDLINK_STATE_API_INIT", __func__);
                vTaskDelay(pdMS_TO_TICKS(5000));
                hidlink_init();
                break;
            }
            
            case HIDLINK_STATE_IDLE: {

                hidlink_command_t command;

                if (xQueueReceive(hidlink.command_queue, &command, pdMS_TO_TICKS(5000)) == pdTRUE) {

                    if (command == HIDLINK_COMMAND_SCAN_START) {
                        if (hidlink.status != HIDLINK_STATUS_SCANNING) {
                            ESP_LOGI(TAG, "%s, HIDLINK_COMMAND_SCAN_START", __func__);
                            hidlink_clear_hid_peripheral_list();
                            hidlink.status = HIDLINK_STATUS_SCANNING;
                            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
                        }
                    }
                    else if (command == HIDLINK_COMMAND_SCAN_STOP) {
                        if (hidlink.status == HIDLINK_STATUS_SCANNING) {
                            hidlink.status = HIDLINK_STATUS_NOT_CONNECTED;
                            ESP_LOGI(TAG, "%s, HIDLINK_COMMAND_SCAN_STOP", __func__);
                            esp_bt_gap_cancel_discovery();
                        }
                    }
                    else if (command == HIDLINK_COMMAND_SCAN_DONE) {
                        ESP_LOGI(TAG, "%s, HIDLINK_COMMAND_SCAN_DONE", __func__);
                        hidlink.status = HIDLINK_STATUS_NOT_CONNECTED;

                        ESP_LOGI(TAG, "hid devices found during scan: %lu", hidlink.hid_peripheral_list.index);
                        
                        if (hidlink.hid_peripheral_list.index != 0) {

                            uint32_t i;
                            char bda_str[18];

                            for(i = 0; i < hidlink.hid_peripheral_list.index; i++) {
                                ESP_LOGI(TAG, "%02lu: %s [%s]", 
                                    i + 1,
                                    hidlink.hid_peripheral_list.device[i].name,
                                    bda2str(hidlink.hid_peripheral_list.device[i].bd_addr, bda_str, 18));
                            }
                        }
                    }
                    else if (command == HIDLINK_COMMAND_SET_STATUS_CONNECTED) {

                        hidlink.status = HIDLINK_STATUS_CONNECTED;
                    }
                    else if (command == HIDLINK_COMMAND_SET_STATUS_IDLE) {
                        
                        hidlink.status = HIDLINK_STATUS_NOT_CONNECTED;
                    }
                    else if (command == HIDLINK_COMMAND_CLEAR_BOND_DEVICE_LIST) {             
                        
                        // remove entire list of bond devices
                        int num_dev = esp_bt_gap_get_bond_device_num();
                        esp_bd_addr_t *bda = NULL;
                        int i;
                    
                        ESP_LOGW(TAG, "num of bond devices: %d", num_dev);

                        bda = (esp_bd_addr_t *) pvPortMalloc(num_dev * sizeof(esp_bd_addr_t));

                        if (bda != NULL) {
                            if (esp_bt_gap_get_bond_device_list(&num_dev, bda) == ESP_OK) {

                                for (i = 0; i < num_dev; i++) {
                                    if (esp_bt_gap_remove_bond_device(bda[i]) == ESP_OK) {
                                        ESP_LOGW(TAG, " - dev %s, remove ok", bda2str(bda[i], bda_str, 18));
                                    }
                                    else {
                                        ESP_LOGW(TAG, " - dev %s, remove error", bda2str(bda[i], bda_str, 18));
                                    }
                                }
                            }
                        }
                        else {
                            ESP_LOGW(TAG, "malloc error");
                        }

                        hidlink.status = HIDLINK_STATUS_NOT_CONNECTED;
                    }
                    else if (command == HIDLINK_COMMAND_SHOW_ATTACHED_DEVICE_INFO) {
                        ESP_LOGI(TAG, "hidlink is attached to device %s [%s]", 
                            hidlink.attached_device.name,
                            bda2str(hidlink.attached_device.bd_addr, bda_str, 18)
                        );    
                    }
                }
                break;
            }        
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

