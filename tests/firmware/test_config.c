/* firmware/storage/config.c: the [devmode] parser and the [firmware] section writer. */
#include "jsb/config.h"
#include "test.h"

static int warnings;
static void count_warning(int line, const char *msg, void *ctx)
{
    (void)line;
    (void)msg;
    (void)ctx;
    warnings++;
}

static jsb_config parse(const char *text)
{
    jsb_config cfg;
    jsb_config_defaults(&cfg);
    warnings = 0;
    jsb_config_parse(text, strlen(text), &cfg, count_warning, NULL);
    return cfg;
}

static void test_default_text(void)
{
    jsb_config defaults, cfg;
    jsb_config_defaults(&defaults);
    cfg = parse(jsb_config_default_text);
    CHECK(warnings == 0);
    CHECK(memcmp(&cfg, &defaults, sizeof cfg) == 0);
    CHECK(cfg.on_crash == JSB_ON_CRASH_RESTART);
    CHECK_CONTAINS(jsb_config_default_text, "on_crash = restart\r\n");
}

static void test_on_crash(void)
{
    jsb_config cfg = parse("[devmode]\non_crash = report\n");
    CHECK(warnings == 0 && cfg.on_crash == JSB_ON_CRASH_REPORT);
    cfg = parse("[devmode]\r\n  on_crash=restart  \r\n");
    CHECK(warnings == 0 && cfg.on_crash == JSB_ON_CRASH_RESTART);
    cfg = parse("[devmode]\non_crash = stop\n");
    CHECK(warnings == 1 && cfg.on_crash == JSB_ON_CRASH_RESTART);
    cfg = parse("[other]\non_crash = report\n"); /* only [devmode] counts */
    CHECK(warnings == 0 && cfg.on_crash == JSB_ON_CRASH_RESTART);
    cfg = parse("[firmware]\nimage_id = 1234abcd\n[devmode]\non_crash = report\nlog_level = debug\n");
    CHECK(warnings == 0 && cfg.on_crash == JSB_ON_CRASH_REPORT && cfg.log_level == JSB_LOG_LEVEL_DEBUG);
}

static void test_other_keys(void)
{
    jsb_config cfg = parse("[devmode]\nusb_drive = none\nsoft_restart = on_eject\nlog_output = both\nbogus = 1\n");
    CHECK(warnings == 1);
    CHECK(cfg.usb_drive == JSB_USB_DRIVE_NONE && cfg.soft_restart == JSB_RESTART_ON_EJECT &&
          cfg.log_output == JSB_LOG_BOTH);
}

static void test_firmware_section(void)
{
    static const char sec_a[] = "[firmware]\r\nimage_id = aaaaaaaa\r\n";
    static const char sec_b[] = "[firmware]\r\nimage_id = bbbbbbbb\r\n";
    char out[512], again[512];

    /* No section yet: goes before the first one. */
    const char *text = "; top\r\n[devmode]\r\non_crash = report\r\n";
    size_t n = jsb_config_set_firmware_section(text, strlen(text), sec_a, out, sizeof out);
    CHECK(n == strlen(text) + strlen(sec_a));
    out[n] = '\0';
    CHECK(strcmp(out, "; top\r\n[firmware]\r\nimage_id = aaaaaaaa\r\n[devmode]\r\non_crash = report\r\n") == 0);

    /* Same image again: unchanged (no new-image boot). */
    size_t m = jsb_config_set_firmware_section(out, n, sec_a, again, sizeof again);
    CHECK(m == n && memcmp(again, out, n) == 0);

    /* Another image: the section changes, the settings stay. */
    m = jsb_config_set_firmware_section(out, n, sec_b, again, sizeof again);
    again[m] = '\0';
    CHECK(m == n && memcmp(again, out, n) != 0);
    CHECK_CONTAINS(again, "image_id = bbbbbbbb");
    CHECK_CONTAINS(again, "[devmode]\r\non_crash = report\r\n");

    /* Doesn't fit: 0. */
    CHECK(jsb_config_set_firmware_section(text, strlen(text), sec_a, out, 10) == 0);
}

int main(void)
{
    test_default_text();
    test_on_crash();
    test_other_keys();
    test_firmware_section();
    return TEST_DONE("test_config");
}
