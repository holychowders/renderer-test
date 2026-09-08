#pragma once

#include <stdint.h>

// SECTION: SIGNED INTS

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

// SECTION: UNSIGNED INTS

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

// SECTION: FLOATS

typedef float F32;
typedef double F64;

// SECTION: CHARS

typedef signed char SChar;
typedef unsigned char UChar;

typedef const char *UTF8;
typedef const uint16_t *UTF16;

// SECTION: BOOLS

#define BOOL_TO_STR(value) ((value) ? "true" : "false")
#ifndef __cplusplus
typedef int32_t B32;
    //typedef int32_t bool;
    #define false 0
    #define true 1
#else
typedef bool B32;
#endif

// SECTION: STRINGS

typedef struct Str8 Str8;

struct Str8 {
    UTF8 str;
    size_t size_bytes;
};

static inline size_t str8_len(UTF8 str) {
    UTF8 ptr = str;
    size_t len = 0;
    while (*ptr++ != '\0') { // Excludes NUL terminator
        len++;
    }
    return len;
}

// Create a Str8 from a NUL-terminated C string
static inline Str8 str8(UTF8 str) { return (Str8){ .str = str, .size_bytes = str8_len(str) }; }

#if 0
typedef struct Str16 Str16;
struct Str16 {
    UTF16 str;
    size_t size_bytes;
};

static inline size_t str16_len(UTF16 str) {
    UTF16 ptr = str;
    size_t len = 0;
    while (*ptr++ != '\0') { // Excludes NUL terminator
        len+=1;
    }
    return len;
}

// Create a Str16 from a NUL-terminated C string
static inline Str16 str16(UTF8 str) {
    return (Str16){.str = str, .size_bytes = 2*str16_len(str)};
}
#endif
