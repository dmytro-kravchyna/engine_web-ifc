// ifc_logger.h
// Single-header C++20 logger with C FFI, mirroring TS API:
// - Enum: LogLevel
// - Functions: log_set_level, log_get_level, log_log, log_debug, log_warn, log_error
//
// How to use:
//   // In ONE .cpp file:
//   #define LOG_HEADER_IMPLEMENTATION
//   #include "ifc_logger.h"
//
//   // Everywhere else:
//   #include "ifc_logger.h"
//
// Notes:
// - Build as C++ (for implementations). C callers consume the extern "C" API.
// - Android needs -llog. Emscripten keeps symbols via EMSCRIPTEN_KEEPALIVE.
// - Works on iOS, Android, Windows, Linux, WebAssembly.
//
// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>
#include <stdint.h>

/* Platform-specific export */
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  /* When building the DLL define WEB_IFC_API_BUILD in your CMake for the target */
  #ifdef WEB_IFC_API_BUILD
    #define FFI_EXPORT __declspec(dllexport)
  #else
    #define FFI_EXPORT __declspec(dllimport)
  #endif
#elif defined(__EMSCRIPTEN__)
  /* Keep the symbol alive through link-time GC */
  #include <emscripten/emscripten.h>
  #define FFI_EXPORT EMSCRIPTEN_KEEPALIVE
#elif defined(__GNUC__) || defined(__clang__)
  /* GCC/Clang (Linux, Android NDK, Apple clang) */
  #define FFI_EXPORT __attribute__((visibility("default")))
#else
  #define FFI_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Single shared enum for C and C++ (reused; no namespaces)
typedef enum LogLevel {
  LOG_LEVEL_DEBUG = 1,
  LOG_LEVEL_WARN  = 3,
  LOG_LEVEL_ERROR = 4,
  LOG_LEVEL_OFF   = 6
} LogLevel;

// TS parity: Log.setLogLevel(level)
FFI_EXPORT void log_set_level(int level);
FFI_EXPORT int  log_get_level(void);

// TS parity: Log.log / Log.debug / Log.warn / Log.error
FFI_EXPORT void log_log(const char* message);
FFI_EXPORT void log_debug(const char* message);
FFI_EXPORT void log_warn(const char* message);
FFI_EXPORT void log_error(const char* message);

#ifdef __cplusplus
} // extern "C"
#endif

// -----------------------------
// Implementation (once)
// -----------------------------
#ifdef __cplusplus
#ifdef LOG_HEADER_IMPLEMENTATION

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
static std::atomic<int> g_ifc_log_level{ LOG_LEVEL_ERROR };

static inline bool ifc_log_allows(int needed) noexcept {
  return g_ifc_log_level.load(std::memory_order_relaxed) <= needed;
}

static void ifc_log_write_line(const char* tag, const char* msg) noexcept {
  if (!msg) return;

#if defined(__ANDROID__)
  int prio = ANDROID_LOG_INFO;
  if (std::strcmp(tag, "ERROR") == 0) prio = ANDROID_log_error;
  else if (std::strcmp(tag, "WARN") == 0) prio = ANDROID_log_warn;
  __android_log_print(prio, "web-ifc", "%s: %s", tag, msg);

#elif defined(__EMSCRIPTEN__)
  if (std::strcmp(tag, "ERROR") == 0) emscripten_console_error(msg);
  else if (std::strcmp(tag, "WARN") == 0) emscripten_console_warn(msg);
  else emscripten_console_log(msg);

#elif defined(_WIN32)
  char buf[2048];
  int n = std::snprintf(buf, sizeof(buf), "%s: %s\r\n", tag, msg);
  if (n > 0) {
    ::OutputDebugStringA(buf);
    std::FILE* stream = (std::strcmp(tag, "ERROR") == 0) ? stderr : stdout;
    std::fwrite(buf, 1, static_cast<size_t>(n), stream);
  }

#else
  std::FILE* stream = (std::strcmp(tag, "ERROR") == 0) ? stderr : stdout;
  std::fprintf(stream, "%s: %s\n", tag, msg);
#endif
}

// ------- extern "C" definitions (exported once) -------
extern "C" {

FFI_EXPORT void log_set_level(int level) {
  g_ifc_log_level.store(level, std::memory_order_relaxed);
}

FFI_EXPORT int log_get_level(void) {
  return g_ifc_log_level.load(std::memory_order_relaxed);
}

FFI_EXPORT void log_log(const char* message) {
  if (ifc_log_allows(LOG_LEVEL_DEBUG)) ifc_log_write_line("LOG", message);
}

FFI_EXPORT void log_debug(const char* message) {
  if (ifc_log_allows(LOG_LEVEL_DEBUG)) ifc_log_write_line("DEBUG", message);
}

FFI_EXPORT void log_warn(const char* message) {
  if (ifc_log_allows(LOG_LEVEL_WARN)) ifc_log_write_line("WARN", message);
}

FFI_EXPORT void log_error(const char* message) {
  if (ifc_log_allows(LOG_LEVEL_ERROR)) ifc_log_write_line("ERROR", message);
}

} // extern "C"

#endif // LOG_HEADER_IMPLEMENTATION
#endif // __cplusplus
