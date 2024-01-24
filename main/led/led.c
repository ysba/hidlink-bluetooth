#include "../main/main.h"
#include <driver/gpio.h>
#include <inttypes.h>

#define GPIO_LED_GREEN      13
#define GPIO_LED_YELLOW     12
#define GPIO_LED_RED        14


typedef struct {
    uint32_t io;
    led_command_t command;
    uint32_t blink;
    uint32_t count;
    bool state;
} led_t;


static led_t led[LED_HANDLE_MAX];


static void led_task() {

    led_handle_t handle = 0;
    bool next = false;

    while (1) {

        switch (led[handle].command) {

            case LED_COMMAND_OFF: {
                gpio_set_level(led[handle].io, 0);
                vTaskDelay(pdMS_TO_TICKS(10));
                next = true;
                break;
            }

            case LED_COMMAND_ON: {
                gpio_set_level(led[handle].io, 1);
                vTaskDelay(pdMS_TO_TICKS(10));
                next = true;
                break;
            }

            case LED_COMMAND_BLINK_INIT: {
                if (led[handle].blink == 0) {
                    gpio_set_level(led[handle].io, 0);
                    vTaskDelay(pdMS_TO_TICKS(10));
                    next = true;
                }
                else {
                    led[handle].command = LED_COMMAND_BLINK_TASK;
                    led[handle].count = led[handle].blink;
                    led[handle].state = true;
                    gpio_set_level(led[handle].io, 1);
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                break;
            }

            case LED_COMMAND_BLINK_TASK: {
                if (led[handle].state == false) {
                    led[handle].state = true;
                    gpio_set_level(led[handle].io, 1);
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                else {
                    led[handle].state = false;
                    gpio_set_level(led[handle].io, 0);
                    if (--led[handle].count == 0) {
                        next = true;
                        led[handle].command = LED_COMMAND_BLINK_INIT;
                    }
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                
                break;
            }
        }

        if (next == true) {
            next = false;
            if (++handle >= LED_HANDLE_MAX) {
                handle = 0;
            }
        }
    }
}


void led_start() {

    gpio_config_t io_conf;
    
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    
    io_conf.pin_bit_mask = BIT(GPIO_LED_GREEN) | BIT(GPIO_LED_YELLOW) | BIT(GPIO_LED_RED);
    gpio_config(&io_conf);

    memset(&led, 0, sizeof(led));

    led[LED_HANDLE_GREEN].io = GPIO_LED_GREEN;
    led[LED_HANDLE_YELLOW].io = GPIO_LED_YELLOW;
    led[LED_HANDLE_RED].io = GPIO_LED_RED;

    
    led_set_off(LED_HANDLE_GREEN);
    led_set_off(LED_HANDLE_YELLOW);
    led_set_off(LED_HANDLE_RED);


    xTaskCreate(led_task, "led", 512, NULL, 10, NULL);
}


void led_set_on(led_handle_t handle) {

    if (handle >= LED_HANDLE_MAX)
        return;

    led[handle].command = LED_COMMAND_ON;
    led[handle].state = true;
    led[handle].blink = 0;
}


void led_set_off(led_handle_t handle) {

    if (handle >= LED_HANDLE_MAX)
        return;

    led[handle].command = LED_COMMAND_OFF;
    led[handle].state = false;
    led[handle].blink = 0;
}


void led_set_blink(led_handle_t handle, uint32_t blink) {

    if (handle >= LED_HANDLE_MAX)
        return;

    led[handle].command = LED_COMMAND_BLINK_INIT;
    led[handle].state = false;
    led[handle].blink = blink;
}
