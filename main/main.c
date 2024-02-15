#include "main.h"

static const char *TAG = "MAIN";


void app_main(void) {

    esp_log_level_set("BT_GAP", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "%s, start", __func__);
    led_start();
    serial_start();
    hidlink_start();
    ESP_LOGI(TAG, "%s, stop", __func__);
    vTaskDelete(NULL);
}
