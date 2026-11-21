/* firmware/core/src/crash.c: the restart decision and the CRASH.TXT text. */
#include "jsb/crash.h"
#include "test.h"

static jsb_crash_info sample(void)
{
    jsb_crash_info c;
    memset(&c, 0, sizeof c);
    c.kind = JSB_CRASH_ASSERT;
    c.lr = 0x10001234u;
    c.uptime_ms = 1500;
    c.count = 1;
    c.image_id = 0xabcdef01u;
    c.line = 42;
    strcpy(c.step, "running the script");
    strcpy(c.msg, "p != NULL");
    strcpy(c.file, "src/gc.c");
    return c;
}

static void test_stop_reason(void)
{
    jsb_crash_info c = sample();
    CHECK(jsb_crash_stop_reason(&c, false) == NULL);
    CHECK(jsb_crash_stop_reason(&c, true) != NULL);
    CHECK_CONTAINS(jsb_crash_stop_reason(&c, true), "on_crash = report");

    c.count = JSB_CRASH_RESTART_MAX - 1;
    CHECK(jsb_crash_stop_reason(&c, false) == NULL);
    c.count = JSB_CRASH_RESTART_MAX;
    CHECK(jsb_crash_stop_reason(&c, false) != NULL);
    CHECK_CONTAINS(jsb_crash_stop_reason(&c, false), "3 times in a row");

    c.count = 1;
    c.new_image = 1; /* first boot after flashing: report whatever the setting */
    CHECK(jsb_crash_stop_reason(&c, false) != NULL);
    CHECK_CONTAINS(jsb_crash_stop_reason(&c, false), "first boot");
}

static void test_format(void)
{
    char out[JSB_CRASH_TEXT_MAX];
    jsb_crash_info c = sample();

    size_t n = jsb_crash_format(&c, c.image_id, NULL, out, sizeof out);
    CHECK(n == strlen(out) && n > 0 && n < sizeof out - 1);
    CHECK_CONTAINS(out, "crash:    assert failed\r\n");
    CHECK_CONTAINS(out, "message:  p != NULL\r\n");
    CHECK_CONTAINS(out, "source:   src/gc.c:42\r\n");
    CHECK_CONTAINS(out, "step:     running the script\r\n");
    CHECK_CONTAINS(out, "lr:       10001234\r\n");
    CHECK_CONTAINS(out, "uptime:   1500 ms\r\n");
    CHECK_CONTAINS(out, "image:    abcdef01\r\n");
    CHECK_CONTAINS(out, "started again");

    c.new_image = 1;
    jsb_crash_format(&c, c.image_id, jsb_crash_stop_reason(&c, false), out, sizeof out);
    CHECK_CONTAINS(out, "(its first boot after flashing)");
    CHECK_CONTAINS(out, "The script was not started (first boot after flashing this firmware).\r\n");
    CHECK_CONTAINS(out, "Reset the board");

    jsb_crash_format(&c, 0x12345678u, NULL, out, sizeof out);
    CHECK_CONTAINS(out, "(not the image on the board now)");

    /* Hard fault: no message or source lines. */
    memset(&c, 0, sizeof c);
    c.kind = JSB_CRASH_FAULT;
    c.pc = 0x20001000u;
    jsb_crash_format(&c, 0, NULL, out, sizeof out);
    CHECK_CONTAINS(out, "crash:    hard fault");
    CHECK(strstr(out, "message:") == NULL && strstr(out, "source:") == NULL);
    CHECK_CONTAINS(out, "step:     -\r\n");
    CHECK_CONTAINS(out, "pc:       20001000\r\n");

    /* Longest fields still fit in JSB_CRASH_TEXT_MAX. */
    c = sample();
    memset(c.step, 'S', sizeof c.step - 1);
    memset(c.msg, 'M', sizeof c.msg - 1);
    memset(c.file, 'F', sizeof c.file - 1);
    c.new_image = 1;
    n = jsb_crash_format(&c, 1, jsb_crash_stop_reason(&c, false), out, sizeof out);
    CHECK(n < sizeof out - 1);
    CHECK_CONTAINS(out, "Reset the board to run it again.\r\n");

    /* Truncation is safe. */
    char small[40];
    n = jsb_crash_format(&c, 1, NULL, small, sizeof small);
    CHECK(n == sizeof small - 1 && strlen(small) == n);
}

int main(void)
{
    test_stop_reason();
    test_format();
    return TEST_DONE("test_crash");
}
