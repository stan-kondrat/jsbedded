/*
 * JerryScript engine adapter. jerry-core only (no jerry-ext, no upstream
 * jerry-port): console.log is installed here, and the jerry_port_* functions
 * the configured core calls are implemented on the HAL below. The JS heap is
 * one static buffer of JSB_JERRYSCRIPT_HEAP_KB (JERRY_GLOBAL_HEAP_SIZE, set per
 * board in engine.cmake) and the context is static, so one runtime exists at
 * a time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "jerryscript.h"
#include "jerryscript-port.h"

#include "jsb/engine.h"
#include "jsb/hal.h"

struct jsb_engine {
    bool active;
};

static jsb_engine the_engine;

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

/* Console output with "\n" turned into "\r\n", as the firmware console expects. */
static void console_write_lines(const char *s, size_t len)
{
    size_t start = 0;

    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n' && (i == 0 || s[i - 1] != '\r')) {
            if (i > start) {
                jsb_hal_console_write(s + start, i - start);
            }
            jsb_hal_console_write("\r\n", 2);
            start = i + 1;
        }
    }
    if (len > start) {
        jsb_hal_console_write(s + start, len - start);
    }
}

/* --- port layer: the jerry_port_* functions jerry-core calls --- */

void jerry_port_init(void)
{
}

/* Out of JS heap is fatal in JerryScript (the static heap cannot grow). */
void jerry_port_fatal(jerry_fatal_code_t code)
{
    const char *what;

    switch (code) {
    case JERRY_FATAL_OUT_OF_MEMORY:
        what = "out of memory";
        break;
    case JERRY_FATAL_REF_COUNT_LIMIT:
        what = "reference count limit";
        break;
    case JERRY_FATAL_UNTERMINATED_GC_LOOPS:
        what = "unterminated GC loops";
        break;
    case JERRY_FATAL_DISABLED_BYTE_CODE:
        what = "disabled byte code";
        break;
    case JERRY_FATAL_FAILED_ASSERTION:
        what = "failed assertion";
        break;
    default:
        what = "unknown";
        break;
    }
    console_print("JSBedded: JerryScript fatal error: ");
    console_print(what);
    console_print("; halted\r\n");
    for (;;) {
        jsb_hal_sleep_ms(1000);
    }
}

void jerry_port_sleep(uint32_t sleep_time)
{
    jsb_hal_sleep_ms(sleep_time);
}

void jerry_port_log(const char *message_p)
{
    console_write_lines(message_p, strlen(message_p));
}

void jerry_port_print_buffer(const jerry_char_t *buffer_p, jerry_size_t buffer_size)
{
    console_write_lines((const char *)buffer_p, buffer_size);
}

/* Date: milliseconds from gettimeofday() (time since boot until a clock source
   sets the time); local time is UTC. */
double jerry_port_current_time(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        return 0.0;
    }
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

int32_t jerry_port_local_tza(double unix_ms)
{
    (void)unix_ms;
    return 0;
}

/* --- console.log --- */

/* Writes a string in chunks, straight from the engine's string storage. */
typedef struct {
    char buf[64];
    size_t len;
    bool lines; /* turn "\n" into "\r\n" */
} chunk_writer;

static void chunk_flush(chunk_writer *w)
{
    if (w->len > 0) {
        if (w->lines) {
            console_write_lines(w->buf, w->len);
        } else {
            jsb_hal_console_write(w->buf, w->len);
        }
        w->len = 0;
    }
}

static void chunk_byte(uint32_t byte, void *user_p)
{
    chunk_writer *w = user_p;

    w->buf[w->len++] = (char)byte;
    if (w->len == sizeof w->buf) {
        chunk_flush(w);
    }
}

/* Write String(value). Returns false if the conversion threw (the exception is
   then returned in *exception if non-NULL, else released). */
static bool write_value(jerry_value_t value, bool lines, jerry_value_t *exception)
{
    jerry_value_t str = jerry_value_to_string(value);
    chunk_writer w = { .len = 0, .lines = lines };

    if (jerry_value_is_exception(str)) {
        if (exception != NULL) {
            *exception = str;
        } else {
            jerry_value_free(str);
        }
        return false;
    }
    jerry_string_iterate(str, JERRY_ENCODING_UTF8, chunk_byte, &w);
    chunk_flush(&w);
    jerry_value_free(str);
    return true;
}

/* console.log(...args): arguments as strings, separated by spaces. */
static jerry_value_t js_console_log(const jerry_call_info_t *call_info_p, const jerry_value_t args_p[],
                                    const jerry_length_t args_count)
{
    (void)call_info_p;
    for (jerry_length_t i = 0; i < args_count; i++) {
        jerry_value_t exception;

        if (i > 0) {
            jsb_hal_console_write(" ", 1);
        }
        if (!write_value(args_p[i], false, &exception)) {
            return exception;
        }
    }
    jsb_hal_console_write("\r\n", 2);
    return jerry_undefined();
}

static bool install_console(void)
{
    jerry_value_t global = jerry_current_realm();
    jerry_value_t console = jerry_object();
    jerry_value_t log = jerry_function_external(js_console_log);
    jerry_value_t r1 = jerry_object_set_sz(console, "log", log);
    jerry_value_t r2 = jerry_object_set_sz(global, "console", console);
    bool ok = !jerry_value_is_exception(r1) && !jerry_value_is_exception(r2);

    jerry_value_free(r2);
    jerry_value_free(r1);
    jerry_value_free(log);
    jerry_value_free(console);
    jerry_value_free(global);
    return ok;
}

/* --- errors --- */

/* Print an uncaught exception and release it: "Uncaught <Name>: <message>",
   then the frames of error.stack ("    at boot.js:1:7"). SyntaxError messages
   carry the position themselves ("Unexpected end of input [boot.js:1:5]"). */
static void print_exception(jerry_value_t exception)
{
    jerry_value_t error = jerry_exception_value(exception, true);

    console_print("Uncaught ");
    if (!write_value(error, true, NULL)) {
        console_print("exception");
    }
    console_print("\r\n");

    if (jerry_value_is_error(error)) {
        jerry_value_t stack = jerry_object_get_sz(error, "stack");

        if (jerry_value_is_array(stack)) {
            jerry_length_t n = jerry_array_length(stack);

            for (jerry_length_t i = 0; i < n; i++) {
                jerry_value_t frame = jerry_object_get_index(stack, i);

                if (!jerry_value_is_exception(frame)) {
                    console_print("    at ");
                    write_value(frame, true, NULL);
                    console_print("\r\n");
                }
                jerry_value_free(frame);
            }
        }
        jerry_value_free(stack);
    }
    jerry_value_free(error);
}

/* --- engine.h --- */

const char *jsb_engine_name(void)
{
    return "jerryscript";
}

jsb_engine *jsb_engine_create(void)
{
    if (the_engine.active) {
        return NULL;
    }
    jerry_init(JERRY_INIT_EMPTY);
    the_engine.active = true;
    if (!install_console()) {
        console_print("JSBedded: JerryScript could not start\r\n");
        jsb_engine_destroy(&the_engine);
        return NULL;
    }
    return &the_engine;
}

int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    jerry_parse_options_t options;
    jerry_value_t result;
    int ret = 0;

    if (engine == NULL || !engine->active) {
        return -1;
    }
    /* jerry_parse() takes a length, so the source needs no NUL. */
    options.options = JERRY_PARSE_HAS_SOURCE_NAME;
    options.source_name = jerry_string_sz(filename != NULL ? filename : "<eval>");
    result = jerry_parse((const jerry_char_t *)source, len, &options);
    jerry_value_free(options.source_name);

    if (!jerry_value_is_exception(result)) {
        jerry_value_t script = result;
        result = jerry_run(script);
        jerry_value_free(script);
    }
    if (jerry_value_is_exception(result)) {
        print_exception(result);
        ret = -1;
    } else {
        jerry_value_free(result);
    }

    /* Run the promise jobs the script queued. jerry_run_jobs() stops at the
       first job that throws, so call it again until the queue is empty. */
    for (;;) {
        jerry_value_t jobs = jerry_run_jobs();
        if (!jerry_value_is_exception(jobs)) {
            jerry_value_free(jobs);
            break;
        }
        print_exception(jobs);
        ret = -1;
    }
    return ret;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL || !engine->active) {
        return;
    }
    jerry_cleanup();
    engine->active = false;
}
