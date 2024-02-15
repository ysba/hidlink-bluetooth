#ifndef __SERIAL_PRIVATE_HEADER__
#define __SERIAL_PRIVATE_HEADER__

#define HIDLINK_UART_PORT_NUM       2
#define HIDLINK_UART_BUF_SIZE       256


typedef enum {
    SERIAL_COMMAND_STATUS_REQUEST = 0x01,
    SERIAL_COMMAND_HID_REPORT = 0x02,
} serial_command_t;


typedef struct {
    serial_status_t status;
    QueueHandle_t queue_tx;
    QueueHandle_t queue_rx;
} serial_t;

void serial_task();
void serial_set_error_state();
serial_frame_t serial_get_status_request_frame();
uint8_t serial_checksum(uint8_t *data, uint32_t len);

#endif