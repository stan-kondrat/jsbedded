/* HAL on ESP-IDF (ESP32-S3, ESP32-C3). Console: UART0 (the dev board's USB-UART port). */
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jsb/core.h"
#include "jsb/hal.h"

void jsb_hal_init(void)
{
}

void jsb_hal_console_write(const char *s, size_t len)
{
    fwrite(s, 1, len, stdout);
    fflush(stdout);
}

uint64_t jsb_hal_millis(void)
{
    return (uint64_t)esp_timer_get_time() / 1000;
}

/* Sleeps at least one tick so the idle task (and its watchdog) can run. */
void jsb_hal_sleep_ms(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    vTaskDelay(ticks > 0 ? ticks : 1);
}

static bool gpio_valid(int pin)
{
    return pin >= 0 && pin < GPIO_NUM_MAX;
}

/* Outputs are configured input+output so jsb_hal_gpio_read() sees the driven level. */
int jsb_hal_gpio_mode(int pin, jsb_gpio_mode mode)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = mode == JSB_GPIO_OUTPUT ? GPIO_MODE_INPUT_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&cfg) == ESP_OK ? 0 : -1;
}

int jsb_hal_gpio_write(int pin, bool high)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    return gpio_set_level((gpio_num_t)pin, high ? 1 : 0) == ESP_OK ? 0 : -1;
}

int jsb_hal_gpio_read(int pin)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    return gpio_get_level((gpio_num_t)pin) ? 1 : 0;
}

/* No dev mode or script drive on ESP32 yet: the BOOT button is not read. */
bool jsb_hal_devmode(void)
{
    return false;
}

void app_main(void)
{
    jsb_main();
}
