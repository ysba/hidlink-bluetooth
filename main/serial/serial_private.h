#ifndef __SERIAL_PRIVATE_HEADER__
#define __SERIAL_PRIVATE_HEADER__

#define HIDLINK_UART_PORT_NUM       2
#define HIDLINK_UART_BUF_SIZE       256


typedef struct {
    serial_status_t status;
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue;
} serial_t;


#endif