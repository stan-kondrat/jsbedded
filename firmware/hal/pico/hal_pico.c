/* HAL on the Pico SDK (RP2040, RP2350). Console: USB serial and UART0. */
#include <stdio.h>

#include "pico/stdlib.h"

#include "jsb/core.h"
#include "jsb/hal.h"

void jsb_hal_init(void)
{
    stdio_init_all();
}

void jsb_hal_console_write(const char *s, size_t len)
{
    fwrite(s, 1, len, stdout);
    fflush(stdout);
}

uint64_t jsb_hal_millis(void)
{
    return time_us_64() / 1000;
}

void jsb_hal_sleep_ms(uint32_t ms)
{
    sleep_ms(ms);
}

static bool gpio_valid(int pin)
{
    return pin >= 0 && (unsigned)pin < NUM_BANK0_GPIOS;
}

/* On the W boards some pins (GP23-25, GP29) belong to the CYW43 chip; they are not blocked. */
int jsb_hal_gpio_mode(int pin, jsb_gpio_mode mode)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, mode == JSB_GPIO_OUTPUT ? GPIO_OUT : GPIO_IN);
    return 0;
}

int jsb_hal_gpio_write(int pin, bool high)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    gpio_put((uint)pin, high);
    return 0;
}

int jsb_hal_gpio_read(int pin)
{
    if (!gpio_valid(pin)) {
        return -1;
    }
    return gpio_get((uint)pin) ? 1 : 0;
}

int main(void)
{
    jsb_main();
}
