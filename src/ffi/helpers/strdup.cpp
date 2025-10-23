#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

// ------------------------- Strings -------------------------

// Copies s to out as a C string (NUL-terminated).
// Returns payload length (no NUL). If out == nullptr, returns required payload size.
// Caller MUST ensure out has at least (return_value + 1) bytes if out != nullptr.
[[nodiscard]] inline std::size_t ffi_strdup(const std::string& s, char* out) noexcept {
  const std::size_t n = s.size();
  if (!out) return n;               // preflight: required payload bytes
  if (n) std::memcpy(out, s.data(), n);
  out[n] = '\0';
  return n;
}

[[nodiscard]] inline std::size_t ffi_strdup(std::string_view s, char* out) noexcept {
  const std::size_t n = s.size();
  if (!out) return n;               // preflight
  if (n) std::memcpy(out, s.data(), n);
  out[n] = '\0';
  return n;
}

// ------------------------- Raw bytes -------------------------

// Copies exactly len bytes from src to out.
// Returns len. If out == nullptr, returns len (required size).
// Caller MUST ensure out has at least len bytes if out != nullptr.
[[nodiscard]] inline std::size_t ffi_strdup(const void* src, std::size_t len, void* out) noexcept {
  if (!out) return len;             // preflight
  if (len) std::memcpy(out, src, len);
  return len;
}

// ------------------------- std::array<T, N> -------------------------

// Copies N elements of trivially copyable T from std::array into out.
// Returns total bytes written (N * sizeof(T)). If out == nullptr, returns required bytes.
// Caller MUST ensure out has at least N elements if out != nullptr.
template <class T, std::size_t N>
[[nodiscard]] inline std::size_t ffi_strdup(const std::array<T, N>& src, T* out) noexcept {
  static_assert(std::is_trivially_copyable_v<T>, "FFI: T must be trivially copyable");
  constexpr std::size_t bytes = sizeof(T) * N;
  if (!out) return bytes;           // preflight
  if (bytes) std::memcpy(out, src.data(), bytes);
  return bytes;
}
