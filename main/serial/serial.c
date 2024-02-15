#include "../main.h"
#include "serial_private.h"

#define IO_UART_TX      GPIO_NUM_17
#define IO_UART_RX      GPIO_NUM_16


static const char *TAG = "SERIAL";

static serial_t serial;


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

    if((err = uart_driver_install(HIDLINK_UART_PORT_NUM, HIDLINK_UART_BUF_SIZE, HIDLINK_UART_BUF_SIZE, 10, &serial.queue_rx, intr_alloc_flags)) != ESP_OK) {
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
    serial.queue_tx = xQueueCreate(10, sizeof(serial_frame_t));

    xTaskCreate(serial_task, "serial", 2048, NULL, 10, NULL);

    ESP_LOGI(TAG, "serial init ok");
}


void serial_task() {

    serial_frame_t frame;
    uart_event_t event;

    while(1) {

        if(xQueueReceive(serial.queue_tx, &frame, pdMS_TO_TICKS(1000)) == pdFALSE) {
            
            frame = serial_get_status_request_frame();
        }

        ESP_LOG_BUFFER_HEX_LEVEL(TAG, frame.data, frame.len, ESP_LOG_DEBUG);
        uart_write_bytes(HIDLINK_UART_PORT_NUM, frame.data, frame.len);
        

        if(xQueueReceive(serial.queue_rx, (void *) &event, pdMS_TO_TICKS(1000)) == pdTRUE) {

            switch (event.type) {

                case UART_DATA: {

                    break;
                }

                case UART_FIFO_OVF: {
                    ESP_LOGW(TAG, "uart event: overflow");
                    uart_flush_input(HIDLINK_UART_PORT_NUM);
                    xQueueReset(serial.queue_rx);
                    break;
                }

                case UART_BUFFER_FULL: {
                    ESP_LOGW(TAG, "uart event: buffer full");
                    uart_flush_input(HIDLINK_UART_PORT_NUM);
                    xQueueReset(serial.queue_rx);
                    break;
                }

                // case UART_BREAK: {
                //     // #TODO: check error event!
                //     ESP_LOGW(TAG, "uart event: break");
                //     break;
                // }

                case UART_FRAME_ERR: {
                    ESP_LOGW(TAG, "uart event: frame error");
                    break;
                }

                default: {
                    ESP_LOGW(TAG, "uart event: unexpected (%d)", event.type);
                    break;
                }
            }
        }
        else {
            serial_set_error_state();
        }
    }
}


void serial_set_error_state() {
    if (serial.status != SERIAL_STATUS_ERROR) {
        serial.status = SERIAL_STATUS_ERROR;
        ESP_LOGW(TAG, "rx timeout");
        led_set_blink(LED_HANDLE_RED, 1);
    }
}


void serial_send_status_request() {

}


void serial_send_hid_report(uint8_t *data, uint32_t len) {

    serial_frame_t frame = {0};
    uint8_t checksum;

    if (len > (SERIAL_FRAME_DATA_LEN - 3)) {
        ESP_LOGW(TAG, "%s, invalid len", __func__);
        return;
    }

    frame.len = 0;
    frame.data[frame.len++] = 0xaa;
    frame.data[frame.len++] = SERIAL_COMMAND_HID_REPORT;
    frame.data[frame.len++] = len;
    memcpy(&frame.data[frame.len], data, len);
    frame.len += len;
    checksum = serial_checksum(frame.data, frame.len);
    frame.data[frame.len++] = checksum;
    
    xQueueSend(serial.queue_tx, &frame, portMAX_DELAY);
}


serial_frame_t serial_get_status_request_frame() {

    serial_frame_t frame = {0};
    uint8_t checksum;

    frame.len = 0;
    frame.data[frame.len++] = 0xaa;
    frame.data[frame.len++] = SERIAL_COMMAND_STATUS_REQUEST;
    frame.data[frame.len++] = 0;
    checksum = serial_checksum(frame.data, frame.len);
    frame.data[frame.len++] = checksum;
    
    return (frame);
}


uint8_t serial_checksum(uint8_t *data, uint32_t len) {

    uint8_t checksum = 0;

    while (len--) {
        checksum += *data++;
    }

    return (~checksum + 1);
}


serial_status_t serial_get_status() {

    return serial.status;
}
