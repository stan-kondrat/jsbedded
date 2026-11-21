/*
 * Crash record (jsb/hal.h) on the Pico SDK. Hard faults, failed asserts,
 * abort(), SDK panics / exit() and jsb_hal_fatal() fill a record in RAM that
 * the C runtime doesn't clear at boot (__uninitialized_ram), then the watchdog
 * restarts the chip. The next boot reads it with jsb_hal_crash_take(). A power
 * cycle loses it; the magic and the checksum tell a record from leftover RAM.
 *
 * Strings are copied into the record: after a reflash, pointers into the old
 * image would point at something else.
 */
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"
#include "hardware/watchdog.h"

#include "jsb/hal.h"

#define CRASH_MAGIC 0x4A534243u /* "JSBC" */

typedef struct {
    uint32_t magic;
    jsb_crash_info info;
    uint32_t check;
} crash_record;

static crash_record __uninitialized_ram(record);
static const char *volatile current_step;
static bool new_image;
static uint32_t image_id; /* jsb_hal_image_id(), 0 until it first ran (in jsb_hal_init) */
static uint32_t streak; /* crashes in a row before this boot */

static uint32_t checksum(const jsb_crash_info *info)
{
    const uint8_t *p = (const uint8_t *)info;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < sizeof *info; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

/* Only copy from flash or RAM: a corrupted pointer must not fault again. */
static bool readable(const void *p)
{
    uintptr_t a = (uintptr_t)p;
    return (a >= XIP_BASE && a < XIP_BASE + 0x01000000u) || (a >= SRAM_BASE && a < SRAM_END);
}

static void copy_str(char *dst, size_t cap, const char *src)
{
    size_t n = 0;
    if (src != NULL) {
        for (; n + 1 < cap && readable(src + n) && src[n] != '\0'; n++) {
            dst[n] = src[n];
        }
    }
    dst[n] = '\0';
}

/* Fill the record; the magic is written last, so a half-written record doesn't count. */
static void save(jsb_crash_kind kind, uint32_t pc, uint32_t lr, uint32_t sp, const char *msg,
                 const char *file, int line)
{
    record.magic = 0;
    memset(&record.info, 0, sizeof record.info);
    jsb_crash_info *c = &record.info;
    c->kind = kind;
    c->pc = pc;
    c->lr = lr;
    c->sp = sp;
    c->uptime_ms = (uint32_t)(time_us_64() / 1000);
    c->count = streak + 1;
    c->image_id = image_id; /* not jsb_hal_image_id(): hashing the image takes too long in a fault */
    c->new_image = new_image;
    c->line = line;
    copy_str(c->step, sizeof c->step, current_step);
    copy_str(c->msg, sizeof c->msg, msg);
    copy_str(c->file, sizeof c->file, file);
    record.check = checksum(c);
    record.magic = CRASH_MAGIC;
}

/*
 * Errors that still run normal code: the watchdog is armed first, so the board
 * restarts even if printing hangs (an assert inside printf, a held stdio lock).
 */
static void __attribute__((noreturn)) save_and_restart(jsb_crash_kind kind, uint32_t lr, const char *what,
                                                       const char *msg, const char *file, int line)
{
    save(kind, 0, lr, 0, msg, file, line);
    watchdog_reboot(0, 0, 500);
    if (file != NULL) {
        printf("JSBedded: %s: %s (%s:%d); restarting\r\n", what, msg != NULL ? msg : "", file, line);
    } else {
        printf("JSBedded: %s%s%s; restarting\r\n", what, msg != NULL ? ": " : "", msg != NULL ? msg : "");
    }
    stdio_flush();
    for (;;) {
        tight_loop_contents();
    }
}

bool jsb_hal_crash_take(jsb_crash_info *info)
{
    crash_record r = record;
    record.magic = 0;
    if (r.magic != CRASH_MAGIC || r.check != checksum(&r.info)) {
        return false;
    }
    r.info.step[sizeof r.info.step - 1] = '\0';
    r.info.msg[sizeof r.info.msg - 1] = '\0';
    r.info.file[sizeof r.info.file - 1] = '\0';
    streak = r.info.count;
    *info = r.info;
    return true;
}

void jsb_hal_crash_step(const char *step)
{
    current_step = step;
}

void jsb_hal_crash_set_new_image(bool yes)
{
    new_image = yes;
}

void jsb_hal_crash_streak_reset(void)
{
    streak = 0;
}

_Noreturn void jsb_hal_fatal(const char *msg, const char *file, int line)
{
    save_and_restart(JSB_CRASH_FATAL, (uint32_t)(uintptr_t)__builtin_return_address(0), "fatal error", msg,
                     file, line);
}

/* newlib's assert() calls this. */
void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    (void)func;
    save_and_restart(JSB_CRASH_ASSERT, (uint32_t)(uintptr_t)__builtin_return_address(0), "assert failed",
                     expr, file, line);
}

void abort(void)
{
    save_and_restart(JSB_CRASH_ABORT, (uint32_t)(uintptr_t)__builtin_return_address(0), "abort()", NULL,
                     NULL, 0);
}

/* The SDK's panic() (out of memory with PICO_MALLOC_PANIC, ...) prints its message, then calls this. */
void _exit(int status)
{
    (void)status;
    save_and_restart(JSB_CRASH_PANIC, (uint32_t)(uintptr_t)__builtin_return_address(0), "panic or exit()",
                     NULL, NULL, 0);
}

/*
 * Hard fault: frame is the exception stack frame (r0-r3, r12, lr, pc, xpsr).
 * The watchdog is armed first, so a second fault while saving (lockup) still
 * ends in a restart.
 */
void __attribute__((noreturn, used)) jsb_pico_hardfault(uint32_t *frame)
{
    watchdog_reboot(0, 0, 50);
    bool ok = readable(frame) && readable(frame + 7);
    save(JSB_CRASH_FAULT, ok ? frame[6] : 0, ok ? frame[5] : 0, (uint32_t)(uintptr_t)frame, NULL, NULL, 0);
    for (;;) {
    }
}

#if defined(__arm__)
/* Pass the stack the fault was taken on (MSP or PSP, bit 2 of EXC_RETURN) to jsb_pico_hardfault. */
void __attribute__((naked)) isr_hardfault(void)
{
    __asm volatile("movs r0, #4\n"
                   "mov r1, lr\n"
                   "tst r0, r1\n"
                   "beq 1f\n"
                   "mrs r0, psp\n"
                   "b 2f\n"
                   "1: mrs r0, msp\n"
                   "2: ldr r1, =jsb_pico_hardfault\n"
                   "bx r1\n");
}
#endif

/* FNV-1a over the words of the flash image (a few ms per 100 KB), computed once, in jsb_hal_init. */
uint32_t jsb_hal_image_id(void)
{
    extern const uint32_t __flash_binary_start, __flash_binary_end;
    if (image_id == 0) {
        uint32_t h = 2166136261u;
        for (const uint32_t *w = &__flash_binary_start; w < &__flash_binary_end; w++) {
            h = (h ^ *w) * 16777619u;
        }
        image_id = h != 0 ? h : 1;
    }
    return image_id;
}
