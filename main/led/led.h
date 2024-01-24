#ifndef __LED_HEADER__
#define __LED_HEADER__


typedef enum {
    LED_HANDLE_GREEN = 0,
    LED_HANDLE_YELLOW,
    LED_HANDLE_RED,
    LED_HANDLE_MAX
} led_handle_t;


typedef enum {
    LED_COMMAND_OFF = 0,
    LED_COMMAND_ON,
    LED_COMMAND_BLINK_INIT,
    LED_COMMAND_BLINK_TASK,
} led_command_t;


void led_start();
void led_set_on(led_handle_t handle);
void led_set_off(led_handle_t handle);
void led_set_blink(led_handle_t handle, uint32_t blink);

#endif
