#ifndef HC_BASIC_H
#define HC_BASIC_H

#define STR_EQ(str1, str2) strcmp((str1), (str2)) == 0
#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])

#define KILOBYTES_TO_BYTES(count) ((count) * (uint64_t)1024)
#define MEGABYTES_TO_BYTES(count) (KILOBYTES_TO_BYTES(count) * 1024)
#define GIGABYTES_TO_BYTES(count) (MEGABYTES_TO_BYTES(count) * 1024)
#define TERABYTES_TO_BYTES(count) (GIGABYTES_TO_BYTES(count) * 1024)

#define RET_IF_EQ(expr, eq)                                                                                                                          \
    do {                                                                                                                                             \
        if ((expr) == (eq)) return eq;                                                                                                               \
    } while (0)
#define RET_IF_0(expr) RET_IF_EQ((expr), 0)

// INTRIN_ALLOCA
#if defined(_MSC_VER)
    #define INTRIN_ALLOCA(size) _alloca(size)
#elif defined(__clang__) || defined(__GNUC__)
    #define INTRIN_ALLOCA(size) __builtin_alloca(size)
#else
    #error "Compiler intrinsic for alloca unavailable."
#endif

/// Returns new length of passed string
static inline size_t str_trim_trailing_newline(char *str, size_t len) {
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
    return len;
}

#endif // HC_BASIC_H
