#ifndef WEB_IFC_LOG_H
#define WEB_IFC_LOG_H

#include <stdarg.h>

/*
 * Cross-platform logging interface.
 * Supports:
 *   - Standard stdout/stderr (default)
 *   - Android logcat (if WEB_IFC_ANDROID_LOG defined or __ANDROID__)
 *   - Custom sink function registration
 *
 * To route all messages through a custom sink, call log_set_sink().
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_OFF   = 6
} LogLevel;

/* Configuration (thread-safe) */
void log_set_level(LogLevel level);
LogLevel log_get_level(void);

/* Sink function type: prefix (e.g., "ERROR: "), formatted message buffer */
typedef void (*log_sink_fn)(const char *levelPrefix, const char *message);

/* Register a custom sink (thread-safe). Pass NULL to restore default behavior */
void log_set_sink(log_sink_fn sink);

/* Convert level enum to constant string (DEBUG/WARN/ERROR/OFF/UNKNOWN). */
const char *log_level_to_string(LogLevel level);

/* Logging functions (printf-style) */
void log_msg(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

/* v* variants accepting an existing va_list (do not call directly with ...). */
void log_vmsg(const char *fmt, va_list ap);
void log_vdebug(const char *fmt, va_list ap);
void log_vwarn(const char *fmt, va_list ap);
void log_verror(const char *fmt, va_list ap);

/* Force flush underlying streams or custom sink (no-op for sink unless user-defined sink flushes). */
void log_flush(void);

/* Optional configuration: maximum stack buffer size (compile-time). */
#ifndef WEB_IFC_LOG_STACK_BUF
#define WEB_IFC_LOG_STACK_BUF 1024
#endif

/* Optional convenience macros with file:line for debug.
 * MSVC does not support '##__VA_ARGS__' so we use portable vararg pattern.
 */
#define LOG_SET_LEVEL(level) log_set_level(level)
#define LOG_GET_LEVEL()      log_get_level()

#if defined(_MSC_VER)
    /* MSVC portable varargs: allow zero variadic args using __VA_OPT__ if available (C++20); fallback requires at least one. */
    #ifdef __cpp_va_opt
        #define LOG_DEBUG(fmt, ...)  log_debug("%s:%d: " fmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
        #define LOG_WARN(fmt, ...)   log_warn(fmt __VA_OPT__(,) __VA_ARGS__)
        #define LOG_ERROR(fmt, ...)  log_error(fmt __VA_OPT__(,) __VA_ARGS__)
        #define LOG_MSG(fmt, ...)    log_msg(fmt __VA_OPT__(,) __VA_ARGS__)
    #else
        #define LOG_DEBUG(fmt, ...)  log_debug("%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)
        #define LOG_WARN(fmt, ...)   log_warn(fmt, __VA_ARGS__)
        #define LOG_ERROR(fmt, ...)  log_error(fmt, __VA_ARGS__)
        #define LOG_MSG(fmt, ...)    log_msg(fmt, __VA_ARGS__)
    #endif
#else
    #define LOG_DEBUG(fmt, ...)  log_debug("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...)   log_warn(fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)  log_error(fmt, ##__VA_ARGS__)
    #define LOG_MSG(fmt, ...)    log_msg(fmt, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* WEB_IFC_LOG_H */