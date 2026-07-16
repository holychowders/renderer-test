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

// ASSERT()
#define ASSERT(expr)                                                                                                                                 \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            ERROR_RE_DETAILED("ASSERT", #expr);                                                                                                      \
            /*fprintf(stderr, "ASRT [%s|L%d|%s]: `%s` (full path: %s)\n", __FILE_NAME__, __LINE__, __FUNCTION__, #expr, __FILE__);*/                 \
            DEBUG_BREAK();                                                                                                                           \
        }                                                                                                                                            \
    } while (0)
