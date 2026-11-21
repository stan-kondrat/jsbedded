/* Minimal test helpers: CHECK keeps going, the test binary exits 1 if any failed. */
#ifndef JSB_TEST_H
#define JSB_TEST_H

#include <stdio.h>
#include <string.h>

static int test_failures;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);      \
            test_failures++;                                                     \
        }                                                                        \
    } while (0)

#define CHECK_CONTAINS(text, part) CHECK(strstr((text), (part)) != NULL)

#define TEST_DONE(name)                                                          \
    (printf("%s: %s\n", (name), test_failures ? "FAILED" : "ok"), test_failures ? 1 : 0)

#endif
