#pragma once
// FFI duplication helpers — single-call "allocate-if-null, otherwise copy".
// - If `out_param` pointer is null: returns required payload size (bytes).
// - If `*out_param` is null: allocates with malloc(...) and fills the buffer.
// - If `*out_param` is non-null: copies into the caller-provided buffer (capacity is the caller's responsibility).
//
// Notes
// - Allocation uses malloc/free for broad FFI compatibility (Dart, Swift/Kotlin, C#, etc.).
// - All functions are noexcept and thread-safe (no shared state).
// - Return value is payload length in **bytes** (for C-strings this excludes the trailing NUL).
//
// Caller must free memory obtained via auto-allocation with `ffi_free(...)` from the same binary.
//
// Empty payloads:
// - Strings: if `*out_param == nullptr`, we still allocate a single NUL byte so the returned C string is non-null.
// - Raw bytes / arrays / vectors: for size 0 we do not allocate and leave `*out_param` unchanged (typically nullptr).

#include <array>
#include <vector>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

// -------------------------------------------------------------------------------------------------
// C strings
// -------------------------------------------------------------------------------------------------
//
// Copies `s` into `*out` as a NUL-terminated C string.
// Behavior:
//   - out == nullptr        → preflight only; returns required payload size (s.size()), no allocation.
//   - *out == nullptr       → allocates (s.size() + 1) bytes via malloc, writes NUL terminator.
//   - *out != nullptr       → copies into caller-provided buffer; caller guarantees capacity >= s.size() + 1.
// Returns: payload length in bytes (excluding the NUL terminator).
[[nodiscard]] inline std::size_t ffi_strdup(const std::string& s, char** out) noexcept {
  const std::size_t n = s.size();
  if (!out) return n; // Preflight

  if (*out == nullptr) {
    // Always allocate at least 1 byte for the NUL terminator, even for empty strings.
    char* p = static_cast<char*>(std::malloc(n + 1));
    if (!p) return 0; // OOM: leave *out unchanged
    *out = p;
  }
  if (n) std::memcpy(*out, s.data(), n);
  (*out)[n] = '\0';
  return n;
}

[[nodiscard]] inline std::size_t ffi_strdup(std::string_view s, char** out) noexcept {
  const std::size_t n = s.size();
  if (!out) return n; // Preflight

  if (*out == nullptr) {
    char* p = static_cast<char*>(std::malloc(n + 1));
    if (!p) return 0; // OOM
    *out = p;
  }
  if (n) std::memcpy(*out, s.data(), n);
  (*out)[n] = '\0';
  return n;
}

// -------------------------------------------------------------------------------------------------
// Raw bytes
// -------------------------------------------------------------------------------------------------
//
// Copies exactly `len` bytes from `src` into `*out`.
// Behavior:
//   - out == nullptr        → preflight; returns `len`, no allocation.
//   - *out == nullptr       → allocates `len` bytes via malloc if len > 0, then copies.
//   - *out != nullptr       → copies into caller-provided buffer; caller guarantees capacity >= len.
// Returns: number of bytes written (== len). If len == 0, returns 0 and does not allocate.
[[nodiscard]] inline std::size_t ffi_memdup(const void* src, std::size_t len, void** out) noexcept {
  if (!out) return len; // Preflight
  if (*out == nullptr) {
    if (len == 0) return 0;
    void* p = std::malloc(len);
    if (!p) return 0; // OOM
    *out = p;
  }
  if (len) std::memcpy(*out, src, len);
  return len;
}

// -------------------------------------------------------------------------------------------------
// std::array<T, N>
// -------------------------------------------------------------------------------------------------
//
// Copies `N` trivially-copyable elements from `src` into `*out`.
// Behavior:
//   - out == nullptr        → preflight; returns N * sizeof(T), no allocation.
//   - *out == nullptr       → allocates N * sizeof(T) (if N > 0), then copies.
//   - *out != nullptr       → copies into caller-provided buffer; caller guarantees capacity >= N elements.
// Returns: total bytes written (N * sizeof(T)).
template <class T, std::size_t N>
[[nodiscard]] inline std::size_t ffi_arrdup(const std::array<T, N>& src, T** out) noexcept {
  static_assert(std::is_trivially_copyable_v<T>, "FFI: T must be trivially copyable");
  constexpr std::size_t bytes = sizeof(T) * N;
  if (!out) return bytes; // Preflight

  if (*out == nullptr) {
    if constexpr (N == 0) return 0;
    void* p = std::malloc(bytes);
    if (!p) return 0; // OOM
    *out = static_cast<T*>(p);
  }
  if constexpr (N > 0) {
    std::memcpy(*out, src.data(), bytes);
  }
  return bytes;
}

// -------------------------------------------------------------------------------------------------
// std::vector<T>
// -------------------------------------------------------------------------------------------------
//
// Copies `vec.size()` trivially-copyable elements from `vec` into `*out` as a contiguous block.
// Behavior:
//   - out == nullptr        → preflight; returns vec.size() * sizeof(T), no allocation.
//   - *out == nullptr       → allocates vec.size() * sizeof(T) (if size > 0), then copies.
//   - *out != nullptr       → copies into caller-provided buffer; caller guarantees capacity >= that many bytes.
// Returns: total bytes written (vec.size() * sizeof(T)).
template <class T>
[[nodiscard]] inline std::size_t ffi_vecdup(const std::vector<T>& vec, T** out) noexcept {
  static_assert(std::is_trivially_copyable_v<T>, "FFI: T must be trivially copyable");
  const std::size_t bytes = sizeof(T) * vec.size();
  if (!out) return bytes; // Preflight

  if (*out == nullptr) {
    if (bytes == 0) return 0;
    void* p = std::malloc(bytes);
    if (!p) return 0; // OOM
    *out = static_cast<T*>(p);
  }
  if (bytes) std::memcpy(*out, vec.data(), bytes);
  return bytes;
}
