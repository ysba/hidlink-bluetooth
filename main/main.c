/**
 * @file main.c
 * @brief This file contains functions for basic arithmetic operations.
 */

#include "main.h"

static const char *TAG = "MAIN";

/**
 * A brief history of JavaDoc-style (C-style) comments.
 *
 * This is the typical JavaDoc-style C-style comment. It starts with two
 * asterisks.
 *
 * @param theory Even if there is only one possible unified theory. it is just a
 *               set of rules and equations.
 */
void app_main(void) {

    esp_log_level_set("BT_GAP", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "%s, start", __func__);
    led_start();
    serial_start();
    hidlink_start();
    ESP_LOGI(TAG, "%s, stop", __func__);
    vTaskDelete(NULL);
}
