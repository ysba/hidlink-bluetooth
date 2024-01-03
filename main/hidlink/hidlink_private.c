#include "../main.h"
#include "hidlink_private.h"


// instance of hidlink_t structure (only visible to hidlink source files)
hidlink_t hidlink;


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