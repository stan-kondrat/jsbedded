/*
 * mJS platform header (CS_PLATFORM == CS_P_CUSTOM, selected in engine.cmake).
 * mJS guesses its platform from compiler defines and has none for pico-sdk;
 * its ESP32 and Unix sections pull in sockets, pthreads and file systems this
 * firmware does not use. Only the libc is needed.
 */
#ifndef JSB_MJS_PLATFORM_CUSTOM_H
#define JSB_MJS_PLATFORM_CUSTOM_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "zu"

#ifndef va_copy
#define va_copy(x, y) __builtin_va_copy(x, y)
#endif

#define CS_ENABLE_STDIO 1
#define MG_ENABLE_FILESYSTEM 0

#endif
