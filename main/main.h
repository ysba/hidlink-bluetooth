#ifndef __MAIN_HEADER__
#define __MAIN_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <sdkconfig.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <esp_bt.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_hidh_api.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>

#include <driver/uart.h>
#include <driver/gpio.h>

#include "hardware.h"
#include "hidlink/hidlink.h"
#include "bt_classic/bt_classic.h"
#include "bt_low_energy/bt_low_energy.h"

#endif
