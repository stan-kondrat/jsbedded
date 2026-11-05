/*
 * Dev mode on the Pico: hold BOOTSEL for 2 s while the firmware runs, then
 * release it. The firmware restarts in dev mode (script drive shown, script not
 * run); doing it again in dev mode restarts normally. The on-board LED lights
 * once 2 s are reached, to say "release now".
 *
 * BOOTSEL isn't a GPIO: it pulls the flash chip select low. Reading it means
 * briefly letting go of the chip select, so no code may run from flash
 * meanwhile: interrupts off and the reader in RAM (from pico-examples,
 * picoboard/button). Holding BOOTSEL while plugging in still starts the ROM's
 * UF2 drive, and so would a restart while it's held, which is why the restart
 * waits for the release.
 *
 * The request crosses the restart in a watchdog scratch register, which a power
 * cycle clears.
 */
#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/time.h"

#include "jsb/hal.h"
#include "pico_hal.h"

#define DEVMODE_MAGIC 0x4a534244u /* "JSBD" */
#define DEVMODE_SCRATCH 0         /* scratch[4..7] belong to the boot ROM */
#define HOLD_MS 2000
#define POLL_MS 50

static bool devmode;
static repeating_timer_t poll_timer;
static uint32_t held_ms;
static bool armed;

static bool __no_inline_not_in_flash_func(bootsel_pressed)(void)
{
    const uint cs_index = 1;
    uint32_t irq = save_and_disable_interrupts();
    hw_write_masked(&ioqspi_hw->io[cs_index].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    for (volatile int i = 0; i < 1000; ++i) {
    }
#if PICO_RP2040
    const uint32_t cs_bit = 1u << 1;
#else
    const uint32_t cs_bit = SIO_GPIO_HI_IN_QSPI_CSN_BITS;
#endif
    bool pressed = !(sio_hw->gpio_hi_in & cs_bit);
    hw_write_masked(&ioqspi_hw->io[cs_index].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    restore_interrupts(irq);
    return pressed;
}

static void led(bool on)
{
#ifdef JSB_BOARD_LED
    gpio_init(JSB_BOARD_LED);
    gpio_set_dir(JSB_BOARD_LED, GPIO_OUT);
    gpio_put(JSB_BOARD_LED, on);
#else
    (void)on;
#endif
}

/* Timer interrupt: no console output here (stdio may be locked by the interrupted code). */
static bool poll(repeating_timer_t *timer)
{
    (void)timer;
    if (bootsel_pressed()) {
        if (!armed) {
            held_ms += POLL_MS;
            if (held_ms >= HOLD_MS) {
                armed = true;
                led(true);
            }
        }
        return true;
    }
    if (armed) {
        watchdog_hw->scratch[DEVMODE_SCRATCH] = devmode ? 0 : DEVMODE_MAGIC;
        watchdog_reboot(0, 0, 10);
        for (;;) {
            tight_loop_contents();
        }
    }
    held_ms = 0;
    return true;
}

void jsb_pico_devmode_init(void)
{
    devmode = watchdog_hw->scratch[DEVMODE_SCRATCH] == DEVMODE_MAGIC;
    watchdog_hw->scratch[DEVMODE_SCRATCH] = 0;
    add_repeating_timer_ms(POLL_MS, poll, NULL, &poll_timer);
}

bool jsb_hal_devmode(void)
{
    return devmode;
}
