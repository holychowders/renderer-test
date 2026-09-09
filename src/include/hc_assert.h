#pragma once

#include "hc_log.h"
#include <stdio.h>

// DEBUG_BREAK()
#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
    #define DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__)
    #define DEBUG_BREAK() __builtin_trap()
#else
    #include <signal.h>
    #if defined(SIGTRAP)
        #define DEBUG_BREAK() raise(SIGTRAP)
    #else
        #include <stdlib.h>
        #define DEBUG_BREAK() abort()
    #endif
#endif

#if 0
    #ifdef __FILE_NAME__
        #define FILE_NAME_ELSE_PATH __FILE_NAME__
    #else
        #define FILE_NAME_ELSE_PATH __FILE__
    #endif
#endif

// ASSERT()
#define ASSERT(expr)                                                                                                                                 \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            fprintf(stderr, "ASRT: %s\n      Location: %s, line %d, function %s (%s)", (#expr), __FILE_NAME__, __LINE__, __FUNCTION__, __FILE__);    \
            DEBUG_BREAK();                                                                                                                           \
        }                                                                                                                                            \
    } while (0)

// ASSERT_MSG() - Raise with an additional message
#define ASSERT_MSG(expr, msg)                                                                                                                        \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            fprintf(stderr,                                                                                                                          \
                    "ASRT: %s\n      Failure: %s\n      Location: %s, line %d, function %s (%s)",                                                    \
                    (msg),                                                                                                                           \
                    (#expr),                                                                                                                         \
                    __FILE_NAME__,                                                                                                                   \
                    __LINE__,                                                                                                                        \
                    __FUNCTION__,                                                                                                                    \
                    __FILE__);                                                                                                                       \
            DEBUG_BREAK();                                                                                                                           \
        }                                                                                                                                            \
    } while (0)

// ASSERT_MSG() - Raise with no expression and just a message
#define ASSERT_RAISE(msg)                                                                                                                            \
    do {                                                                                                                                             \
        fprintf(stderr, "ASRT: %s\n      Location: %s, line %d, function %s (%s)", (msg), __FILE_NAME__, __LINE__, __FUNCTION__, __FILE__);          \
        DEBUG_BREAK();                                                                                                                               \
    } while (0)
