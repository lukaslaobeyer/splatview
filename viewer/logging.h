#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

inline void
__attribute__ ((format (printf, 1, 2)))
LOG_DEBUG(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("DEBUG: ", stdout);
    vfprintf(stdout, fmt, args);
    fputs("\n", stdout);
    va_end(args);
}

inline void
__attribute__ ((format (printf, 1, 2)))
LOG_INFO(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("INFO: ", stdout);
    vfprintf(stdout, fmt, args);
    fputs("\n", stdout);
    va_end(args);
}

inline void
__attribute__ ((format (printf, 1, 2)))
LOG_ERROR(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("ERROR: ", stderr);
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);
    va_end(args);
}

[[noreturn]]
inline void
__attribute__ ((format (printf, 1, 2)))
LOG_FATAL(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("FATAL: ", stderr);
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);
    va_end(args);

    std::abort();
}

namespace logging {
void print_progress(double progress);
}
