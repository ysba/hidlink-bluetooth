#include "../main.h"
#include "bt_classic_private.h"


static const char *TAG = "BT_CLASSIC";


bool bt_classic_init() {

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    bool ret_val = true;
    esp_err_t err;

    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed:  %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init_with_cfg failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_gap_register_callback(bt_gap_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_gap_register_callback failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_hid_host_register_callback(bt_hid_host_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_hid_host_register_callback failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_gap_set_scan_mode failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_hid_host_init()) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_hid_host_init failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_bt_dev_set_device_name(dev_name)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_dev_set_device_name failed: %s", esp_err_to_name(err));
        ret_val = false;
    }

    return (ret_val);
}


char *bda2str(esp_bd_addr_t bda, char *str, size_t size) {
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}