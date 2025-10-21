#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
#include <android/log.h>
#endif

static atomic_int currentLogLevel = ATOMIC_VAR_INIT(LOG_LEVEL_ERROR);
static _Atomic(log_sink_fn) currentSink = NULL; /* Custom sink or NULL */

void log_set_level(LogLevel level) {
    atomic_store(&currentLogLevel, (int)level);
}

LogLevel log_get_level(void) {
    return (LogLevel)atomic_load(&currentLogLevel);
}

void log_set_sink(log_sink_fn sink) {
    atomic_store(&currentSink, sink);
}

static void dispatch_message(const char *levelPrefix, const char *formatted) {
    log_sink_fn sink = atomic_load(&currentSink);
    if (sink) {
        sink(levelPrefix, formatted);
        return;
    }
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
    int androidPrio = ANDROID_LOG_INFO;
    if (strncmp(levelPrefix, "ERROR", 5) == 0) androidPrio = ANDROID_LOG_ERROR;
    else if (strncmp(levelPrefix, "WARN", 4) == 0) androidPrio = ANDROID_LOG_WARN;
    else if (strncmp(levelPrefix, "DEBUG", 5) == 0) androidPrio = ANDROID_LOG_DEBUG;
    __android_log_print(androidPrio, "web-ifc", "%s%s", levelPrefix, formatted);
#else
    FILE *stream = stdout;
    if (strncmp(levelPrefix, "ERROR", 5) == 0 || strncmp(levelPrefix, "WARN", 4) == 0) {
        stream = stderr;
    }
    if (levelPrefix && *levelPrefix) {
        fputs(levelPrefix, stream);
    }
    fputs(formatted, stream);
    fputc('\n', stream);
#endif
}

static bool format_is_safe(const char *fmt) {
    /* Disallow %n to prevent writing to arbitrary memory via format string */
    const char *p = fmt;
    while ((p = strchr(p, '%')) != NULL) {
        p++; /* move past % */
        if (*p == '%') { p++; continue; } /* escaped percent */
        /* Skip flags, width, precision, length until conversion specifier */
        while (*p && strchr("-+ #0", *p)) p++;
        while (*p && (*p >= '0' && *p <= '9')) p++;
        if (*p == '.') {
            p++; while (*p && (*p >= '0' && *p <= '9')) p++;
        }
        if (strchr("hljztL", *p)) { p++; }
        if (*p == 'n') return false; /* unsafe */
        if (*p) p++; /* advance past specifier */
    }
    return true;
}

const char *log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_OFF:   return "OFF";
        default: return "UNKNOWN";
    }
}

static void vbuild_and_dispatch(const char *prefix, const char *fmt, va_list ap) {
    if (!format_is_safe(fmt)) {
        dispatch_message("ERROR: ", "Unsafe format string (contains %n) blocked");
        return;
    }
    char stackBuf[WEB_IFC_LOG_STACK_BUF];
    va_list apCopy;
    va_copy(apCopy, ap);
    int needed = vsnprintf(stackBuf, sizeof(stackBuf), fmt, apCopy);
    va_end(apCopy);
    if (needed < 0) {
        return; /* formatting error */
    }
    if (needed < (int)sizeof(stackBuf)) {
        dispatch_message(prefix, stackBuf);
    } else {
        size_t size = (size_t)needed + 1;
        char *dyn = (char*)malloc(size);
        if (!dyn) {
            /* Fallback to truncated stackBuf */
            dispatch_message(prefix, stackBuf);
            return;
        }
        va_list apCopy2;
        va_copy(apCopy2, ap);
        vsnprintf(dyn, size, fmt, apCopy2);
        va_end(apCopy2);
        dispatch_message(prefix, dyn);
        free(dyn);
    }
}

void log_msg(const char *fmt, ...) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_ERROR) {
        va_list ap;
        va_start(ap, fmt);
        vbuild_and_dispatch("", fmt, ap);
        va_end(ap);
    }
}

void log_vmsg(const char *fmt, va_list ap) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_ERROR) {
        vbuild_and_dispatch("", fmt, ap);
    }
}

void log_debug(const char *fmt, ...) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_DEBUG) {
        va_list ap;
        va_start(ap, fmt);
        vbuild_and_dispatch("DEBUG: ", fmt, ap);
        va_end(ap);
    }
}

void log_vdebug(const char *fmt, va_list ap) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_DEBUG) {
        vbuild_and_dispatch("DEBUG: ", fmt, ap);
    }
}

void log_warn(const char *fmt, ...) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_WARN) {
        va_list ap;
        va_start(ap, fmt);
        vbuild_and_dispatch("WARN: ", fmt, ap);
        va_end(ap);
    }
}

void log_vwarn(const char *fmt, va_list ap) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_WARN) {
        vbuild_and_dispatch("WARN: ", fmt, ap);
    }
}

void log_error(const char *fmt, ...) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_ERROR) {
        va_list ap;
        va_start(ap, fmt);
        vbuild_and_dispatch("ERROR: ", fmt, ap);
        va_end(ap);
    }
}

void log_verror(const char *fmt, va_list ap) {
    if (atomic_load(&currentLogLevel) <= LOG_LEVEL_ERROR) {
        vbuild_and_dispatch("ERROR: ", fmt, ap);
    }
}

void log_flush(void) {
    /* If a custom sink is installed we cannot assume flush semantics. */
    if (atomic_load(&currentSink) == NULL) {
        fflush(stdout);
        fflush(stderr);
    }
#if defined(__ANDROID__) || defined(WEB_IFC_ANDROID_LOG)
    /* Android logcat flush is not typically needed; placeholder if future buffering added. */
#endif
}