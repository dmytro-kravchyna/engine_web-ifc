
cmake_minimum_required(VERSION 3.20)

#
# CMake build for the C FFI layer of the WebIFC library.
#
# This CMakeLists.txt lives in the `ffi` subdirectory of the project.
# It builds a small C wrapper around the underlying C++ implementation
# found in the sibling `cpp` directory (if present).  The wrapper
# exports a C API compatible with multiple platforms, including iOS,
# Android, Linux, macOS, Windows and WebAssembly (Emscripten).  When
# the C++ project is absent, the build still succeeds because a stub
# `ModelManager` header is provided under `cpp/web-ifc/modelmanager`.

project(web_ifc_api
  VERSION 0.1.0
  LANGUAGES C CXX)

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(BUILD_TESTING "Build unit tests" ON)
option(WEB_IFC_API_BUILD_SHARED "Build web_ifc_api as a shared library" OFF)

# Path to the sibling C++ project.  If the real C++ sources are present
# alongside this directory they can be built by passing a valid target
# name via WEBIFC_CPP_TARGET.  Otherwise, a stub ModelManager header is
# used to satisfy the include in web_ifc_api.cpp.
set(WEBIFC_CPP_DIR "${CMAKE_CURRENT_LIST_DIR}/../cpp" CACHE PATH
    "Path to the sibling web-ifc C++ project (with its own CMake)")

set(WEBIFC_CPP_TARGET "" CACHE STRING
    "Exact CMake target name to link from the cpp project (optional)")

# Always generate position independent code.  This is required on
# mobile platforms (iOS/Android) and safe elsewhere.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect target platform.
set(IS_EMSCRIPTEN ${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
# On Android NDK builds the variable ANDROID is defined (by toolchain).
# Using CMAKE_SYSTEM_NAME alone is unreliable on some toolchains, so
# derive IS_ANDROID directly from the ANDROID variable.
set(IS_ANDROID ${ANDROID})

set(IS_IOS OFF)
if(APPLE)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IS_IOS ON)
  elseif(DEFINED CMAKE_OSX_SYSROOT AND CMAKE_OSX_SYSROOT MATCHES "iphone")
    set(IS_IOS ON)
  endif()
endif()

if(WEBIFC_CPP_TARGET)
  message(STATUS "Resolved WEBIFC_CPP_TARGET = '${WEBIFC_CPP_TARGET}', will link against this C++ target.")
else()
  message(STATUS "No C++ target detected; building wrapper against stub ModelManager.")
endif()

# -----------------------------------------------------------------------------
# Bring in the sibling C++ project, if present
# -----------------------------------------------------------------------------
if(EXISTS "${WEBIFC_CPP_DIR}/CMakeLists.txt")
  message(STATUS "Found sibling C++ project at: ${WEBIFC_CPP_DIR} — adding as subdirectory _webifc_cpp_build")
  # Isolate the build artifacts of the C++ project in a subdirectory
  add_subdirectory("${WEBIFC_CPP_DIR}" _webifc_cpp_build)
else()
  message(STATUS "Sibling C++ project not found at: ${WEBIFC_CPP_DIR} — building wrapper only (stub ModelManager header will be used if present)")
endif()

# Probe for a target to link against if the user didn't provide one.
if(NOT WEBIFC_CPP_TARGET)
  # Try a few common names.  The first existing target wins.
  foreach(cand IN ITEMS
      web_ifc_cpp web-ifc-library webifc webifc_core webifc-lib)
    if(TARGET ${cand})
      set(WEBIFC_CPP_TARGET "${cand}")
      break()
    endif()
  endforeach()
endif()

# -----------------------------------------------------------------------------
# Sources and headers
# -----------------------------------------------------------------------------
set(WEB_IFC_API_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/web_ifc_api.cpp
)

set(WEB_IFC_API_PUBLIC_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/web_ifc_api.h
  ${CMAKE_CURRENT_LIST_DIR}/helpers/log.h
  ${CMAKE_CURRENT_LIST_DIR}/helpers/numeric.h
  ${CMAKE_CURRENT_LIST_DIR}/ifc_schema.h
)

# -----------------------------------------------------------------------------
# Library: web_ifc_api
# -----------------------------------------------------------------------------
set(_libtype STATIC)
if(WEB_IFC_API_BUILD_SHARED)
  set(_libtype SHARED)
endif()

add_library(web_ifc_api ${_libtype}
  ${WEB_IFC_API_SOURCES}
  ${WEB_IFC_API_PUBLIC_HEADERS}
)

# Emit a log message at build time showing the location of the generated
# library.  We use a post‑build custom command with `cmake -E echo`
# instead of a configure‑time message so that the generator expression
# `$<TARGET_FILE:...>` is resolved after the file actually exists.  This
# helps users to see exactly where the output archive or shared library
# has been placed in the build tree.
add_custom_command(TARGET web_ifc_api POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "[web_ifc_api] built library at: $<TARGET_FILE:web_ifc_api>"
)

# Public include directories.  These paths allow consumers to simply
# `#include <web_ifc_api.h>` and find all necessary helpers.  We also
# expose the sibling C++ project directory for the implementation to
# find additional headers if the real C++ sources are available.
target_include_directories(web_ifc_api
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/helpers>
    $<BUILD_INTERFACE:${WEBIFC_CPP_DIR}>
    $<INSTALL_INTERFACE:include/web_ifc_api>
)

# Link against the C++ target if it exists.  If no C++ target was
# detected, this quietly does nothing; the stub ModelManager header
# defined in cpp/web-ifc/modelmanager/ModelManager.h suffices to
# satisfy the compilation of web_ifc_api.cpp.
if(WEBIFC_CPP_TARGET)
  target_link_libraries(web_ifc_api PRIVATE ${WEBIFC_CPP_TARGET})
endif()

# Platform-specific tweaks
#
# Only link against the Android system log library when building for
# Android.  CMake's `ANDROID` variable is defined by the NDK toolchain
# when targeting that platform, so use it directly here.  This avoids
# attempting to link a non‑existent `liblog` on desktop hosts (macOS,
# Linux, Windows), which would cause linker errors like "ld: library
# 'log' not found".
if(ANDROID)
  # Link against Android's logcat for __android_log_print used by log.h
  target_link_libraries(web_ifc_api PRIVATE log)
endif()

if(MSVC)
  # Force UTF-8 source encoding and strict mode on MSVC
  target_compile_options(web_ifc_api PRIVATE /utf-8 /permissive-)
else()
  # Enable a reasonable set of warnings on GCC/Clang
  target_compile_options(web_ifc_api PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(IS_EMSCRIPTEN)
  # Emscripten link flags to allow memory growth and tune memory usage
  target_link_options(web_ifc_api PRIVATE
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s MAXIMUM_MEMORY=4GB"
    "SHELL:-s STACK_SIZE=5MB"
  )
endif()

# Provide a namespaced alias for consumers
add_library(web-ifc::api ALIAS web_ifc_api)
message(STATUS "Created target alias 'web-ifc::api' -> web_ifc_api")

# -----------------------------------------------------------------------------
# Unit tests (C++ executables)
#
# The unit tests originally written in C have been translated to C++ in
# the `test` subdirectory (log_test.cpp, numeric_test.cpp,
# ifc_schema_test.cpp).  Compiling them as C++ avoids name clashes
# between the generated C schema header and the C++ schema from the
# upstream project when both are present.  Tests are built whenever
# BUILD_TESTING is enabled and run via `ctest` or the `run_tests`
# custom target.
# -----------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  # Always run the test suite after building.  By attaching the run
  # command to a custom target with the ALL property, tests will be
  # executed as part of the default build on every platform.  If a
  # platform cannot execute the binaries (e.g., cross‑compiled mobile
  # builds), the user may disable BUILD_TESTING entirely.  Otherwise,
  # tests are always run.

  # Test executables compiled as C++
  set(TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/log_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test/numeric_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test/ifc_schema_test.cpp
  )

  foreach(src IN LISTS TEST_SOURCES)
    # Derive target name from filename (strip directory and extension)
    get_filename_component(tgt_name ${src} NAME_WE)
    add_executable(${tgt_name} ${src})
    # Include paths: the ffi root and helpers for test headers
    target_include_directories(${tgt_name} PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/helpers
    )
    # Link tests to the C API library
    target_link_libraries(${tgt_name} PRIVATE web_ifc_api)
    # Use strict warnings for tests
    if(MSVC)
      target_compile_options(${tgt_name} PRIVATE /utf-8)
    else()
      target_compile_options(${tgt_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    # Register the test with CTest
    add_test(NAME ${tgt_name} COMMAND ${tgt_name})
    # Post‑build message showing where each test binary was generated.
    add_custom_command(TARGET ${tgt_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "[${tgt_name}] built executable at: $<TARGET_FILE:${tgt_name}>"
    )
  endforeach()

  # Link against libm on Unix for numeric_test.cpp (C++ may still need libm)
  if(UNIX AND NOT APPLE AND NOT IS_EMSCRIPTEN)
    target_link_libraries(numeric_test PRIVATE m)
  endif()

  # Create a custom target that runs all tests.  By specifying
  # ALL here, the run_tests target becomes part of the default build,
  # ensuring that tests execute automatically after the test binaries
  # have been built.
  add_custom_target(run_tests ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS log_test numeric_test ifc_schema_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
  )
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# -----------------------------------------------------------------------------
# Installation
#
# Exporting a CMake package requires that all dependency targets are part of
# the same export set.  When linking against the upstream C++ library
# (WEBIFC_CPP_TARGET), that target is not under our control.  Attempting
# to install an export set referencing it would cause configuration errors
# ("requires target \"web-ifc-library\" that is not in any export set").
#
# Therefore, only generate an export file when this wrapper is being built
# standalone (i.e. without linking to the real C++ project).  When a real
# C++ target is linked, we still install the library and headers but
# skip exporting a CMake package.  Downstream projects can link the
# installed library with `find_library` instead of `find_package` in that case.

if(NOT WEBIFC_CPP_TARGET)
  # Standalone build: export the web_ifc_api target for find_package use.
  install(TARGETS web_ifc_api
    EXPORT web_ifc_apiTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES ${WEB_IFC_API_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/web_ifc_api)

  install(EXPORT web_ifc_apiTargets
    NAMESPACE web-ifc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/web_ifc_api)
else()
  # Linked build: install library and headers without exporting a package.
  install(TARGETS web_ifc_api
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${WEB_IFC_API_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/web_ifc_api)
endif()