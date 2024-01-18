#include "../main.h"
#include "bt_low_energy_private.h"

static const char *TAG = "BT_LOW_ENERGY";


bool bt_low_energy_init() {

    bool ret_val = true;
    esp_err_t err;

    if ((err = esp_ble_gatts_register_callback(ble_gatts_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gatts_register_callback failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_ble_gap_register_callback(ble_gap_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_ble_gatts_app_register(ESP_SPP_APP_ID)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gatts_app_register failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_ble_gap_config_scan_rsp_data_raw(
            ble_gap_get_scan_data(),
            ble_gap_get_scan_len())) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_config_scan_rsp_data_raw failed: %s", esp_err_to_name(err));
        ret_val = false;
    }
    else if ((err = esp_ble_gap_config_adv_data_raw(
            ble_gap_get_advertising_data(),
            ble_gap_get_advertising_len())) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_config_scan_rsp_data_raw failed: %s", esp_err_to_name(err));
        ret_val = false;
    } 

    return (ret_val);
}