#ifndef __SERIAL_HEADER__
#define __SERIAL_HEADER__


#define SERIAL_FRAME_DATA_LEN   32


typedef enum {
    SERIAL_STATUS_INIT = 0,
    SERIAL_STATUS_OK,
    SERIAL_STATUS_ERROR
} serial_status_t;


typedef struct {
    uint32_t len;
    uint8_t data[SERIAL_FRAME_DATA_LEN];
} serial_frame_t;


void serial_start();
serial_status_t serial_get_status();
void serial_send_hid_report(uint8_t *data, uint32_t len);


#endif