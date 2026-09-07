#ifndef HC_LOG_H
#define HC_LOG_H

#include <stdio.h>
#include <stdarg.h>

////////////////////////////////////////////////////////////////////////// SECTION(PUBLIC): MACROS

#define ERROR_RE_DETAILED(re, msg) ferror_re((re), "%s in %s() on line %d of %s (%s)", (msg), __FUNCTION__, __LINE__, __FILE_NAME__, __FILE__)
#define ERROR_DETAILED(msg) ferror("%s in %s() on line %d of %s (%s)", (msg), __FUNCTION__, __LINE__, __FILE_NAME__, __FILE__)

////////////////////////////////////////////////////////////////////////// SECTION(PUBLIC): DECLARATIONS

static inline void marker(void);

static inline void info(const char *msg);
static inline void warn(const char *msg);
static inline void error(const char *msg);

static inline void info_re(const char *re, const char *msg);
static inline void warn_re(const char *re, const char *msg);
static inline void error_re(const char *re, const char *msg);

__attribute__((format(printf, 1, 2))) static inline void finfo(const char *fmt_msg, ...);
__attribute__((format(printf, 1, 2))) static inline void fwarn(const char *fmt_msg, ...);

__attribute__((format(printf, 2, 3))) static inline void finfo_re(const char *re, const char *fmt_msg, ...);
__attribute__((format(printf, 2, 3))) static inline void fwarn_re(const char *re, const char *fmt_msg, ...);
__attribute__((format(printf, 2, 3))) static inline void ferror_re(const char *re, const char *fmt_msg, ...);

////////////////////////////////////////////////////////////////////////// SECTION(PRIVATE): DECLARATIONS

__attribute__((format(printf, 2, 0))) static inline void _vinfo(const char *re, const char *fmt_msg, va_list fmt_args);
__attribute__((format(printf, 2, 0))) static inline void _vwarn(const char *re, const char *fmt_msg, va_list fmt_args);
__attribute__((format(printf, 2, 0))) static inline void _verror(const char *re, const char *fmt_msg, va_list fmt_args);

////////////////////////////////////////////////////////////////////////// SECTION(PUBLIC): DEFINITIONS

static inline void marker(void) {
    puts("MARKER: ***************************************************************************\n");
    fflush(stdout);
}

static inline void info(const char *msg) { info_re(NULL, msg); }
static inline void warn(const char *msg) { warn_re(NULL, msg); }
static inline void error(const char *msg) { error_re(NULL, msg); }

static inline void info_re(const char *re, const char *msg) {
    if (re) { fprintf(stdout, "INFO [%s]: %s\n", re, msg); }
    else { fprintf(stdout, "INFO: %s\n", msg); }
    fflush(stdout);
}
static inline void warn_re(const char *re, const char *msg) {
    if (re) { fprintf(stderr, "WARN [%s]: %s\n", re, msg); }
    else { fprintf(stderr, "WARN: %s\n", msg); }
}
static inline void error_re(const char *re, const char *msg) {
    if (re) { fprintf(stderr, "FAIL [%s]: %s\n", re, msg); }
    else { fprintf(stderr, "FAIL: %s\n", msg); }
}

__attribute__((format(printf, 1, 2))) static inline void finfo(const char *fmt_msg, ...) {
    va_list args = { 0 };
    va_start(args, fmt_msg);
    _vinfo(NULL, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 1, 2))) static inline void fwarn(const char *fmt_msg, ...) {
    va_list args = { 0 };
    va_start(args, fmt_msg);
    _vwarn(NULL, fmt_msg, args);
    va_end(args);
}

__attribute__((format(printf, 2, 3))) static inline void finfo_re(const char *re, const char *fmt_msg, ...) {
    va_list args = { 0 };
    va_start(args, fmt_msg);
    _vinfo(re, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 2, 3))) static inline void fwarn_re(const char *re, const char *fmt_msg, ...) {
    va_list args = { 0 };
    va_start(args, fmt_msg);
    _vwarn(re, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 2, 3))) static inline void ferror_re(const char *re, const char *fmt_msg, ...) {
    va_list args = { 0 };
    va_start(args, fmt_msg);
    _verror(re, fmt_msg, args);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////// SECTION(PRIVATE): DEFINITIONS

__attribute__((format(printf, 2, 0))) static inline void _vinfo(const char *re, const char *fmt_msg, va_list fmt_args) {
    if (re) { printf("INFO [%s]: ", re); }
    else { printf("INFO: "); }
    vprintf(fmt_msg, fmt_args);
    fputc('\n', stdout);
    fflush(stdout);
}
__attribute__((format(printf, 2, 0))) static inline void _vwarn(const char *re, const char *fmt_msg, va_list fmt_args) {
    if (re) { fprintf(stderr, "WARN [%s]: ", re); }
    else { fprintf(stderr, "WARN: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}
__attribute__((format(printf, 2, 0))) static inline void _verror(const char *re, const char *fmt_msg, va_list fmt_args) {
    if (re) { fprintf(stderr, "FAIL [%s]: ", re); }
    else { fprintf(stderr, "FAIL: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}

#endif // HC_LOG_H

#if 0

static inline void info_stderr(const char *regarding, const char *description);
static inline void error(const char *regarding, const char *description, const char *fpath, int lineno);
static inline void error(const char *description, const char *fpath, int lineno);

static inline void info_stderr(const char *description);

static inline void info_stderr(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "INFO [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "INFO: %s\n", description); }
}
static inline void error(const char *regarding, const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d] [%s]: %s\n", fpath, lineno, regarding, description);
}
static inline void error(const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d]: %s\n", fpath, lineno, description);
}

static inline void info_stderr(const char *description) {
    info_stderr(NULL, description);
}

#endif
