/*
 * <inttypes.h> wrapper for engine builds: includes the C library's header,
 * then defines the 64-bit printf format macros if it left them out.
 *
 * Ubuntu's arm-none-eabi newlib (pico-sdk builds) leaves PRId64 and friends
 * undefined, so engines that format 64-bit values fail to compile. Engines that
 * need this put firmware/engines/include/compat on their include path. int64_t
 * is long long on every JSBedded target, which the assertion checks.
 */
#ifndef JSB_COMPAT_INTTYPES_H
#define JSB_COMPAT_INTTYPES_H

#include_next <inttypes.h>
#include <stdint.h>

#ifndef __cplusplus
_Static_assert(sizeof(int64_t) == sizeof(long long), "int64_t must be long long");
#endif

#ifndef PRId64
#define PRId64 "lld"
#endif
#ifndef PRIi64
#define PRIi64 "lli"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIx64
#define PRIx64 "llx"
#endif
#ifndef PRIX64
#define PRIX64 "llX"
#endif
#ifndef PRIo64
#define PRIo64 "llo"
#endif

#endif
