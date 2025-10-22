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




typedef enum {
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_OFF   = 6
} LogLevel;

/* Sink function type: prefix (e.g., "ERROR: "), formatted message buffer */
typedef void (*log_sink_fn)(const char *levelPrefix, const char *message);

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

/*
 * Implementation below. The extern "C" block remains open until the end of
 * the file so that the definitions also have C linkage when included
 * in C++ translation units. The include guard is not closed until after
 * the implementation to ensure the implementation is only included once.
 */

/* ======================================================================= */
/*  Header-only implementation                                             */
/*                                                                         */
/*  The following section provides a complete implementation of the        */
/*  logging functions declared above.  All state is held in static        */
/*  variables with internal linkage, so each translation unit including    */
/*  this header will have its own logging state.  This ensures that the   */
/*  header can be included without requiring a separate .c file.          */
/*                                                                         */
/*  The implementation uses atomic operations for thread safety and       */
/*  supports Android logcat when either __ANDROID__ or                    */
/*  WEB_IFC_ANDROID_LOG is defined.                                       */
/* ======================================================================= */

#ifdef __cplusplus
extern "C" {
#endif

/* Include additional headers needed by the implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
#include <android/log.h>
#endif

/* When building with Emscripten (WebAssembly), include the
 * appropriate headers so that we can route log messages to the
 * JavaScript console.  These headers define the emscripten_log()
 * function and the EM_LOG_* flag constants, as well as convenience
 * wrappers like emscripten_console_log().  See the Emscripten API
 * reference for details【360683100230186†L49-L70】.
 */
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/console.h>
#endif

/* Internal static state for log level and custom sink.  Each translation  */
/* unit including this header will have its own copy.                     */
static atomic_int web_ifc_log_currentLogLevel = ATOMIC_VAR_INIT(LOG_LEVEL_ERROR);
static _Atomic(log_sink_fn) web_ifc_log_currentSink = NULL;

/* Set the current log level.  Only messages at or below this level will  */
/* be emitted.                                                            */
static inline void log_set_level(LogLevel level) {
    atomic_store(&web_ifc_log_currentLogLevel, (int)level);
}

/* Get the current log level. */
static inline LogLevel log_get_level(void) {
    return (LogLevel)atomic_load(&web_ifc_log_currentLogLevel);
}

/* Register a custom sink function.  When a sink is registered, all       */
/* messages are forwarded to it rather than written to stdout/stderr or   */
/* Android's logcat.  Pass NULL to restore the default behaviour.        */
static inline void log_set_sink(log_sink_fn sink) {
    atomic_store(&web_ifc_log_currentSink, sink);
}

/* Convert a LogLevel enum to a human‑readable string. */
static inline const char *log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_OFF:   return "OFF";
        default: return "UNKNOWN";
    }
}

/* Internal helper to dispatch a message either to the custom sink,        */
/* Android logcat, or the appropriate standard stream.                    */
static inline void web_ifc_log_dispatch_message(const char *levelPrefix, const char *formatted) {
    log_sink_fn sink = atomic_load(&web_ifc_log_currentSink);
    if (sink) {
        sink(levelPrefix, formatted);
        return;
    }
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
    int androidPrio = ANDROID_LOG_INFO;
    /* Map our log prefixes to Android log priorities.  If the prefix
     * indicates an error or warning, choose the corresponding Android
     * priority so that the message appears at the appropriate severity
     * in logcat.  Debug defaults to ANDROID_LOG_DEBUG and all other
     * messages default to ANDROID_LOG_INFO.
     */
    if (strncmp(levelPrefix, "ERROR", 5) == 0) {
        androidPrio = ANDROID_LOG_ERROR;
    } else if (strncmp(levelPrefix, "WARN", 4) == 0) {
        androidPrio = ANDROID_LOG_WARN;
    } else if (strncmp(levelPrefix, "DEBUG", 5) == 0) {
        androidPrio = ANDROID_LOG_DEBUG;
    }
    __android_log_print(androidPrio, "web-ifc", "%s%s", levelPrefix, formatted);

#elif defined(__EMSCRIPTEN__)
    /* When targeting WebAssembly via Emscripten, use emscripten_log() to
     * send messages directly to the browser's JavaScript console.  The
     * EM_LOG_CONSOLE flag ensures the message goes directly to the
     * console, bypassing Module.out()/err().  We choose the log level
     * flag based on our prefix so that errors, warnings and debug
     * messages are distinguished appropriately【607520833762944†L1674-L1703】.  For
     * plain messages (log_msg), treat them as info-level logs.
     */
    int emFlags = EM_LOG_CONSOLE;
    if (strncmp(levelPrefix, "ERROR", 5) == 0) {
        emFlags |= EM_LOG_ERROR;
    } else if (strncmp(levelPrefix, "WARN", 4) == 0) {
        emFlags |= EM_LOG_WARN;
    } else if (strncmp(levelPrefix, "DEBUG", 5) == 0) {
        emFlags |= EM_LOG_DEBUG;
    } else {
        emFlags |= EM_LOG_INFO;
    }
    /* Compose the message from the prefix and the formatted string. */
    emscripten_log(emFlags, "%s%s", levelPrefix, formatted);

#else
    /* Default native implementation: write to stdout/stderr.  For
     * warnings and errors, use stderr; everything else goes to
     * stdout.  Prefixes are written without newlines, followed by
     * the formatted message and a newline. */
    FILE *stream = stdout;
    if ((strncmp(levelPrefix, "ERROR", 5) == 0) ||
        (strncmp(levelPrefix, "WARN", 4) == 0)) {
        stream = stderr;
    }
    if (levelPrefix && *levelPrefix) {
        fputs(levelPrefix, stream);
    }
    fputs(formatted, stream);
    fputc('\n', stream);
#endif
}

/* Detect whether a format string is safe.  Rejects use of the %n         */
/* conversion specifier, which could otherwise write to arbitrary memory. */
static inline bool web_ifc_log_format_is_safe(const char *fmt) {
    const char *p = fmt;
    while ((p = strchr(p, '%')) != NULL) {
        p++; /* move past % */
        if (*p == '%') {
            p++; /* escaped percent */
            continue;
        }
        /* Skip flags, width, precision, and length fields. */
        while (*p && strchr("-+ #0", *p)) p++;
        while (*p && (*p >= '0' && *p <= '9')) p++;
        if (*p == '.') {
            p++;
            while (*p && (*p >= '0' && *p <= '9')) p++;
        }
        if (strchr("hljztL", *p)) {
            p++;
        }
        if (*p == 'n') {
            return false; /* Unsafe: %n detected */
        }
        if (*p) {
            p++; /* advance past specifier */
        }
    }
    return true;
}

/* Build a message from a printf‑style format string and dispatch it.      */
static inline void web_ifc_log_vbuild_and_dispatch(const char *prefix, const char *fmt, va_list ap) {
    if (!web_ifc_log_format_is_safe(fmt)) {
        web_ifc_log_dispatch_message("ERROR: ", "Unsafe format string (contains %n) blocked");
        return;
    }

    char stackBuf[WEB_IFC_LOG_STACK_BUF];
    va_list apCopy;
    va_copy(apCopy, ap);
    int needed = vsnprintf(stackBuf, sizeof(stackBuf), fmt, apCopy);
    va_end(apCopy);
    if (needed < 0) {
        /* Formatting failed – do nothing. */
        return;
    }
    if (needed < (int)sizeof(stackBuf)) {
        /* Fits in the stack buffer. */
        web_ifc_log_dispatch_message(prefix, stackBuf);
    } else {
        /* Allocate a buffer large enough for the formatted message. */
        size_t size = (size_t)needed + 1;
        char *dyn = (char *)malloc(size);
        if (!dyn) {
            /* Allocation failed; fall back to truncated stackBuf. */
            web_ifc_log_dispatch_message(prefix, stackBuf);
            return;
        }
        va_list apCopy2;
        va_copy(apCopy2, ap);
        vsnprintf(dyn, size, fmt, apCopy2);
        va_end(apCopy2);
        web_ifc_log_dispatch_message(prefix, dyn);
        free(dyn);
    }
}

/* Public logging functions.  These check the current log level and       */
/* forward to vbuild_and_dispatch if appropriate.                         */
static inline void log_msg(const char *fmt, ...) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_ERROR) {
        va_list ap;
        va_start(ap, fmt);
        web_ifc_log_vbuild_and_dispatch("", fmt, ap);
        va_end(ap);
    }
}

static inline void log_vmsg(const char *fmt, va_list ap) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_ERROR) {
        web_ifc_log_vbuild_and_dispatch("", fmt, ap);
    }
}

static inline void log_debug(const char *fmt, ...) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_DEBUG) {
        va_list ap;
        va_start(ap, fmt);
        web_ifc_log_vbuild_and_dispatch("DEBUG: ", fmt, ap);
        va_end(ap);
    }
}

static inline void log_vdebug(const char *fmt, va_list ap) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_DEBUG) {
        web_ifc_log_vbuild_and_dispatch("DEBUG: ", fmt, ap);
    }
}

static inline void log_warn(const char *fmt, ...) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_WARN) {
        va_list ap;
        va_start(ap, fmt);
        web_ifc_log_vbuild_and_dispatch("WARN: ", fmt, ap);
        va_end(ap);
    }
}

static inline void log_vwarn(const char *fmt, va_list ap) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_WARN) {
        web_ifc_log_vbuild_and_dispatch("WARN: ", fmt, ap);
    }
}

static inline void log_error(const char *fmt, ...) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_ERROR) {
        va_list ap;
        va_start(ap, fmt);
        web_ifc_log_vbuild_and_dispatch("ERROR: ", fmt, ap);
        va_end(ap);
    }
}

static inline void log_verror(const char *fmt, va_list ap) {
    if (atomic_load(&web_ifc_log_currentLogLevel) <= LOG_LEVEL_ERROR) {
        web_ifc_log_vbuild_and_dispatch("ERROR: ", fmt, ap);
    }
}

/* Force a flush of the underlying streams if no custom sink is           */
/* registered.  On Android this is a no-op as logcat does not buffer.     */
static inline void log_flush(void) {
    if (atomic_load(&web_ifc_log_currentSink) == NULL) {
        fflush(stdout);
        fflush(stderr);
    }
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
    /* Placeholder for potential future buffering behaviour. */
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WEB_IFC_LOG_H */
