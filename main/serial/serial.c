/**
 * @file serial.c
 * @brief Serial communication between ESP32 and RP2040.
 */

#include "../main.h"
#include "serial_private.h"

#define IO_UART_TX      GPIO_NUM_17
#define IO_UART_RX      GPIO_NUM_16


static const char *TAG = "SERIAL";

serial_t serial;


static void serial_set_error_state() {
    if (serial.status != SERIAL_STATUS_ERROR) {
        serial.status = SERIAL_STATUS_ERROR;
        ESP_LOGW(TAG, "rx timeout");
        led_set_blink(LED_HANDLE_RED, 1);
    }
}


static void serial_task() {

    serial_frame_t frame;

    while(1) {

        // step 1: send command wheter via queue or timeout
        if(xQueueReceive(serial.tx_queue, &frame, pdMS_TO_TICKS(200)) == pdTRUE) {
            // send hid report

            //uart_write_bytes(HIDLINK_UART_PORT_NUM, buf, tx_count);

            //ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, tx_count, ESP_LOG_INFO);   
        }
        else {
            // send polling request
        }

        // step 2: wait for response
        if(xQueueReceive(serial.rx_queue, (void *) &event, pdMS_TO_TICKS(50)) == pdTRUE) {

        }
        else {
            serial_set_error_state();
        }
    }
}


void serial_start() {

    esp_err_t err;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = ESP_INTR_FLAG_IRAM;

    if((err = uart_driver_install(HIDLINK_UART_PORT_NUM, HIDLINK_UART_BUF_SIZE, 0, 0, &serial.rx_queue, intr_alloc_flags)) != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return;
    }
    else if((err = uart_param_config(HIDLINK_UART_PORT_NUM, &uart_config)) != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return;
    }
    else if((err = uart_set_pin(HIDLINK_UART_PORT_NUM, IO_UART_TX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return;
    }
    else if((err = uart_set_mode(HIDLINK_UART_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX)) != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_mode failed: %s", esp_err_to_name(err));
        return;
    }

    serial.status = SERIAL_STATUS_INIT;
    serial.tx_queue = xQueueCreate(10, sizeof(serial_frame_t));

    xTaskCreate(serial_task, "serial", 2048, NULL, 10, NULL);

    ESP_LOGI(TAG, "serial init ok");
}

/**
 * @brief Send HID report via serial port.
 *
 * This function is called by the Bluetooth stack whenever a HID report is received. 
 * The HID report is then sent to the USB interface via UART.
 *
 * @param data HID report data
 * @param len HID report length
 */
void serial_send_hid_report(uint8_t *data, uint32_t len) {

    serial_frame_t frame = {0};

    uint32_t i;
    uint8_t checksum = 0;

    // 0xaa
    // len (max = sizeof(buf) - 3)
    // data[n]
    // checksum

    if (len > (SERIAL_FRAME_DATA_LEN - 3)) {
        ESP_LOGW(TAG, "%s, invalid len", __func__);
        return;
    }

    frame.data[frame.len++] = 0xaa;
    frame.data[frame.len++] = len;
    memcpy(&frame.data[frame.len], data, len);
    frame.len += len;

    // checksum
    frame.data[frame.len] = 0;
    for (i = 0; i < frame.len; i++) {
        frame.data[frame.len] += frame.data[i];
    }

    frame.data[frame.len++] = ~checksum + 1;

    xQueueSend(serial.tx_queue, &frame, portMAX_DELAY);
}