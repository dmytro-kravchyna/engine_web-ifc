// Implementation for the logging API declared in helpers/log.h
// This file provides the single translation unit that defines the
// log_* functions. Link this object into the library so tests and
// consumers use the same implementation.

#include "helpers/log.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#if defined(__ANDROID__)
#include <android/log.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten/console.h>
#endif

// Global log level
static std::atomic<int> g_ifc_log_level{LOG_LEVEL_ERROR};

static inline bool ifc_log_allows(int needed) noexcept
{
    return g_ifc_log_level.load(std::memory_order_relaxed) <= needed;
}

static void ifc_log_write_line(const char *tag, const char *msg) noexcept
{
    if (!msg)
        return;

#if defined(__ANDROID__)
    int prio = ANDROID_LOG_INFO;
    if (std::strcmp(tag, "ERROR") == 0)
        prio = ANDROID_LOG_ERROR;
    else if (std::strcmp(tag, "WARN") == 0)
        prio = ANDROID_LOG_WARN;
    __android_log_print(prio, "web-ifc", "%s: %s", tag, msg);

#elif defined(__EMSCRIPTEN__)
    if (std::strcmp(tag, "ERROR") == 0)
        emscripten_console_error(msg);
    else if (std::strcmp(tag, "WARN") == 0)
        emscripten_console_warn(msg);
    else
        emscripten_console_log(msg);

#elif defined(_WIN32)
    char buf[2048];
    int n = std::snprintf(buf, sizeof(buf), "%s: %s\r\n", tag, msg);
    if (n > 0)
    {
        ::OutputDebugStringA(buf);
        std::FILE *stream = (std::strcmp(tag, "ERROR") == 0) ? stderr : stdout;
        std::fwrite(buf, 1, static_cast<size_t>(n), stream);
    }

#else
    std::FILE *stream = (std::strcmp(tag, "ERROR") == 0) ? stderr : stdout;
    std::fprintf(stream, "%s: %s\n", tag, msg);
#endif
}

extern "C" FFI_EXPORT void log_set_level(LogLevel level)
{
    // Store as integer under the hood; LogLevel is compatible with int values.
    g_ifc_log_level.store(static_cast<int>(level), std::memory_order_relaxed);
}

extern "C" FFI_EXPORT int log_get_level(void)
{
    return g_ifc_log_level.load(std::memory_order_relaxed);
}

extern "C" FFI_EXPORT void log_log(const char *message)
{
    if (ifc_log_allows(LOG_LEVEL_DEBUG))
        ifc_log_write_line("LOG", message);
}

extern "C" FFI_EXPORT void log_debug(const char *message)
{
    if (ifc_log_allows(LOG_LEVEL_DEBUG))
        ifc_log_write_line("DEBUG", message);
}

extern "C" FFI_EXPORT void log_warn(const char *message)
{
    if (ifc_log_allows(LOG_LEVEL_WARN))
        ifc_log_write_line("WARN", message);
}

extern "C" FFI_EXPORT void log_error(const char *message)
{
    if (ifc_log_allows(LOG_LEVEL_ERROR))
        ifc_log_write_line("ERROR", message);
}
