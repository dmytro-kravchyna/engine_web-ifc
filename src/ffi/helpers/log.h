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
FFI_EXPORT void log_set_level(LogLevel level);
FFI_EXPORT int  log_get_level(void);

// TS parity: Log.log / Log.debug / Log.warn / Log.error
FFI_EXPORT void log_log(const char* message);
FFI_EXPORT void log_debug(const char* message);
FFI_EXPORT void log_warn(const char* message);
FFI_EXPORT void log_error(const char* message);

#ifdef __cplusplus
} // extern "C"
#endif

// Implementation is provided in a dedicated translation unit `helpers/log.cpp`.
// Include that file in exactly one .cpp when building the library/executable.

// Note: keep declarations in this header only. The implementation depends on
// C++ headers and must not be present in multiple translation units to avoid
// duplicate symbol errors.
