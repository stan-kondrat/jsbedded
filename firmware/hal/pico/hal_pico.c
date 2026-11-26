/*
 * HAL on the Pico SDK (RP2040, RP2350). Console: USB serial and UART0.
 * Also here: the script drive (flash_disk.c, usb_drive.c), dev mode (devmode.c)
 * and the crash record (crash.c).
 */
#include <stdio.h>

#include "pico/stdlib.h"

/*
 * The main stack is the 8 KB of scratch RAM at the top of the address space;
 * below it a deeper stack would run into the heap. JSB_PICO_STACK_RESERVE
 * (bytes, set by engines that need a deeper stack) keeps that much main RAM
 * free for it: the heap stops short of it. Replaces the SDK's weak _sbrk.
 */
#ifndef JSB_PICO_STACK_RESERVE
#define JSB_PICO_STACK_RESERVE 0
#endif

void *_sbrk(int incr)
{
    extern char end, __StackLimit;
    static char *heap_end;
    char *limit = &__StackLimit - JSB_PICO_STACK_RESERVE;
    if (heap_end == NULL) {
        heap_end = &end;
    }
    if (incr > limit - heap_end) {
#ifdef JSB_TRACE
        printf("trace: sbrk FAILED incr=%d heap_end=%08lx limit=%08lx\r\n", incr, (unsigned long)(uintptr_t)heap_end,
               (unsigned long)(uintptr_t)limit);
#endif
        return (void *)-1;
    }
    char *prev = heap_end;
    heap_end += incr;
    return prev;
}

#ifdef JSB_TRACE
#include <malloc.h>
#include "pico/stdio_usb.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "jsb/hal.h"

#define TRACE_STACK_LO (0x20040000u - JSB_PICO_STACK_RESERVE)
#define TRACE_PAINT 0x5AA5C33Cu

static unsigned trace_stack_max(void)
{
    uint32_t *w = (uint32_t *)TRACE_STACK_LO;
    while ((uintptr_t)w < 0x20042000u && *w == TRACE_PAINT) {
        w++;
    }
    return 0x20042000u - (unsigned)(uintptr_t)w;
}

void jsb_trace(const char *what)
{
    volatile char here;
    jsb_hal_crash_step(what);
    struct mallinfo mi = mallinfo();
    printf("trace: %-22s sp=%08lx stack_max=%u heap_arena=%u heap_used=%u\r\n", what,
           (unsigned long)(uintptr_t)&here, trace_stack_max(), (unsigned)mi.arena, (unsigned)mi.uordblks);
    stdio_flush();
    sleep_ms(50);
}

struct _reent;
void *__real__malloc_r(struct _reent *r, size_t size);
void *__wrap__malloc_r(struct _reent *r, size_t size)
{
    static bool busy;
    void *p = __real__malloc_r(r, size);
    if (size >= 1024 && !busy) {
        busy = true;
        printf("trace: alloc %u -> %08lx (from %08lx)\r\n", (unsigned)size, (unsigned long)(uintptr_t)p,
               (unsigned long)(uintptr_t)__builtin_return_address(0));
        busy = false;
    }
    return p;
}

static void trace_start(void)
{
    volatile char here;
    for (uint32_t *w = (uint32_t *)TRACE_STACK_LO; (uintptr_t)w < (uintptr_t)&here - 128; w++) {
        *w = TRACE_PAINT;
    }
    uint32_t fsp = watchdog_hw->scratch[1], fpc = watchdog_hw->scratch[2], flr = watchdog_hw->scratch[3];
    watchdog_hw->scratch[1] = watchdog_hw->scratch[2] = watchdog_hw->scratch[3] = 0;
    for (int i = 0; i < 200 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    sleep_ms(300);
    printf("\r\ntrace: diagnostic build, waiting done (connected=%d)\r\n", (int)stdio_usb_connected());
    if ((fsp >> 24) == 0x20) {
        printf("trace: PREVIOUS BOOT HARD FAULT: pc=%08lx lr=%08lx sp=%08lx\r\n", (unsigned long)fpc,
               (unsigned long)flr, (unsigned long)fsp);
    }
    jsb_trace("hal init");
}
#endif /* JSB_TRACE */

#include "jsb/core.h"
#include "jsb/hal.h"
#include "pico_hal.h"

void jsb_hal_init(void)
{
    jsb_pico_devmode_init();
    stdio_init_all();
    jsb_hal_image_id(); /* computed now: a crash record needs it, and a fault can't wait for it */
#ifdef JSB_TRACE
    trace_start();
#endif

    uint32_t firmware_kb, drive_kb;
    if (!jsb_pico_drive_check(&firmware_kb, &drive_kb) && drive_kb > 0) {
        printf("JSBedded: no script drive: the firmware (%lu KB) reaches into the last %lu KB of flash "
               "set aside for it\r\n", (unsigned long)firmware_kb, (unsigned long)drive_kb);
    }
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
