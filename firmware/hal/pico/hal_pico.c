/* HAL on the Pico SDK (RP2040, RP2350). Console: USB serial and UART0. */
#include <stdio.h>

#include "pico/stdlib.h"

#include "jsb/core.h"
#include "jsb/hal.h"

void jsb_hal_init(void)
{
    stdio_init_all();
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void jsb_hal_console_write(const char *s, size_t len)
{
    fwrite(s, 1, len, stdout);
    fflush(stdout);
}

void jsb_hal_sleep_ms(uint32_t ms)
{
    sleep_ms(ms);
}

/* Pico W boards wire the LED to the CYW43 chip; that needs the cyw43 driver (later). */
void jsb_hal_led_set(bool on)
{
#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#else
    (void)on;
#endif
}

int main(void)
{
    jsb_main();
}
