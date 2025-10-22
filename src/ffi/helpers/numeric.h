/* numeric_only.h
   Portable numeric-only tagged union with wide type coverage, string parsing,
   and optional C11 _Generic helper for "any pointer" inputs.
   Platforms: iOS, Android (NDK), Windows, Linux, macOS.
   Language: C99+ (the _Generic helper requires C11; a typed fallback is provided).
*/
#ifndef NUMERIC_ONLY_H
#define NUMERIC_ONLY_H

/* ----- Standard headers ----- */
#include <stdint.h>   /* int*_t, uint*_t, intptr_t, uintptr_t, intmax_t, uintmax_t */
#include <stddef.h>   /* size_t, ptrdiff_t */
#include <stdlib.h>   /* strtod */
#include <errno.h>    /* errno, ERANGE */
#include <math.h>     /* NAN (fallback below if missing) */

#ifdef __cplusplus
extern "C" {
#endif

/* ----- Fallback for NAN on very old C libraries ----- */
#ifndef NAN
  #define NAN (0.0/0.0)
#endif

/* ----- Optional 128-bit integer support (GCC/Clang; typically not MSVC) -----
   You can force-disable with: #define NUMERIC_ONLY_DISABLE_INT128 1 before including.
*/
#if !defined(NUMERIC_ONLY_DISABLE_INT128)
  #if defined(__SIZEOF_INT128__) && !defined(_MSC_VER)
    #define NUMERIC_ONLY_HAVE_INT128 1
    typedef __int128            numeric_only_int128_t;
    typedef unsigned __int128   numeric_only_uint128_t;
  #else
    #define NUMERIC_ONLY_HAVE_INT128 0
  #endif
#else
  #define NUMERIC_ONLY_HAVE_INT128 0
#endif

/* ----- Detect C11 _Generic availability for the convenience macro ----- */
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
  #define NUMERIC_ONLY_HAVE_GENERIC 1
#elif defined(__clang__)
  #if __has_extension(c_generic_selections)
    #define NUMERIC_ONLY_HAVE_GENERIC 1
  #endif
#elif defined(__GNUC__)
  #if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    #define NUMERIC_ONLY_HAVE_GENERIC 1
  #endif
#endif
#ifndef NUMERIC_ONLY_HAVE_GENERIC
  #define NUMERIC_ONLY_HAVE_GENERIC 0
#endif

/* ----- Inline control (override if embedding in a DLL) ----- */
#ifndef NUMERIC_ONLY_INLINE
  #define NUMERIC_ONLY_INLINE static inline
#endif

/* ============================== Types ============================== */

typedef enum {
    /* Fixed-width signed/unsigned */
    NUMERIC_I8,  NUMERIC_U8,
    NUMERIC_I16, NUMERIC_U16,
    NUMERIC_I32, NUMERIC_U32,
    NUMERIC_I64, NUMERIC_U64,

    /* Native integer families */
    NUMERIC_INT,      NUMERIC_UINT,
    NUMERIC_LONG,     NUMERIC_ULONG,
    NUMERIC_LLONG,    NUMERIC_ULLONG,

    /* Pointer-sized / max-width / differences */
    NUMERIC_INTPTR,   NUMERIC_UINTPTR,
    NUMERIC_SIZE,     NUMERIC_PTRDIFF,
    NUMERIC_IMAX,     NUMERIC_UMAX,

    /* Floating point */
    NUMERIC_F32, NUMERIC_F64, NUMERIC_F128,

#if NUMERIC_ONLY_HAVE_INT128
    /* Optional 128-bit integers */
    NUMERIC_I128, NUMERIC_U128,
#endif
} NumericTag;

/* Numeric-only tagged union. */
typedef struct {
    NumericTag tag;
    union {
        /* Fixed-width */
        int8_t     i8;   uint8_t    u8;
        int16_t    i16;  uint16_t   u16;
        int32_t    i32;  uint32_t   u32;
        int64_t    i64;  uint64_t   u64;

        /* Native families */
        int        i;    unsigned int       ui;
        long       l;    unsigned long      ul;
        long long  ll;   unsigned long long ull;

        /* Pointer-sized / max-width / differences */
        intptr_t   ip;   uintptr_t  uip;
        size_t     zs;   ptrdiff_t  pd;
        intmax_t   im;   uintmax_t  um;

        /* Floating point */
        float       f32;
        double      f64;
        long double f128;

#if NUMERIC_ONLY_HAVE_INT128
        numeric_only_int128_t   i128;
        numeric_only_uint128_t  u128;
#endif
    } as;
} Numeric;

/* ============================ Constructors ============================ */
/* Fixed-width */
NUMERIC_ONLY_INLINE Numeric numeric_from_i8 (int8_t  v){ Numeric n; n.tag=NUMERIC_I8;  n.as.i8 =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_u8 (uint8_t v){ Numeric n; n.tag=NUMERIC_U8;  n.as.u8 =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_i16(int16_t v){ Numeric n; n.tag=NUMERIC_I16; n.as.i16=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_u16(uint16_t v){Numeric n; n.tag=NUMERIC_U16; n.as.u16=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_i32(int32_t v){ Numeric n; n.tag=NUMERIC_I32; n.as.i32=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_u32(uint32_t v){Numeric n; n.tag=NUMERIC_U32; n.as.u32=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_i64(int64_t v){ Numeric n; n.tag=NUMERIC_I64; n.as.i64=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_u64(uint64_t v){Numeric n; n.tag=NUMERIC_U64; n.as.u64=v; return n; }

/* Native families */
NUMERIC_ONLY_INLINE Numeric numeric_from_int  (int v)               { Numeric n; n.tag=NUMERIC_INT;   n.as.i   =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_uint (unsigned v)          { Numeric n; n.tag=NUMERIC_UINT;  n.as.ui  =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_long (long v)              { Numeric n; n.tag=NUMERIC_LONG;  n.as.l   =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_ulong(unsigned long v)     { Numeric n; n.tag=NUMERIC_ULONG; n.as.ul  =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_llong(long long v)         { Numeric n; n.tag=NUMERIC_LLONG; n.as.ll  =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_ullong(unsigned long long v){Numeric n; n.tag=NUMERIC_ULLONG;n.as.ull =v; return n; }

/* Pointer-sized / max-width / differences */
NUMERIC_ONLY_INLINE Numeric numeric_from_intptr (intptr_t v)  { Numeric n; n.tag=NUMERIC_INTPTR;  n.as.ip =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_uintptr(uintptr_t v) { Numeric n; n.tag=NUMERIC_UINTPTR; n.as.uip=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_size   (size_t v)    { Numeric n; n.tag=NUMERIC_SIZE;    n.as.zs =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_ptrdiff(ptrdiff_t v) { Numeric n; n.tag=NUMERIC_PTRDIFF; n.as.pd =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_imax   (intmax_t v)  { Numeric n; n.tag=NUMERIC_IMAX;    n.as.im =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_umax   (uintmax_t v) { Numeric n; n.tag=NUMERIC_UMAX;    n.as.um =v; return n; }

/* Floating point */
NUMERIC_ONLY_INLINE Numeric numeric_from_f32 (float v)        { Numeric n; n.tag=NUMERIC_F32;  n.as.f32 =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_f64 (double v)       { Numeric n; n.tag=NUMERIC_F64;  n.as.f64 =v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_f128(long double v)  { Numeric n; n.tag=NUMERIC_F128; n.as.f128=v; return n; }

#if NUMERIC_ONLY_HAVE_INT128
NUMERIC_ONLY_INLINE Numeric numeric_from_i128(numeric_only_int128_t v)  { Numeric n; n.tag=NUMERIC_I128; n.as.i128=v; return n; }
NUMERIC_ONLY_INLINE Numeric numeric_from_u128(numeric_only_uint128_t v) { Numeric n; n.tag=NUMERIC_U128; n.as.u128=v; return n; }
#endif

/* ====================== Parse from C string ======================= */
/* Parses a C string via strtod (locale-dependent).
   - If s is NULL or parsing fails/overflows, returns NUMERIC_F64 with NaN.
   - On success, returns NUMERIC_F64 with the parsed value.
   - If ok!=NULL, *ok is set to 1 on success, 0 on failure.
*/
NUMERIC_ONLY_INLINE Numeric numeric_from_string(const char* s, int* ok) {
    if (ok) *ok = 0;
    if (!s) return numeric_from_f64(NAN);
    errno = 0;
    char* endp = NULL;
    double d = strtod(s, &endp);
    if (endp == s || errno == ERANGE) return numeric_from_f64(NAN);
    if (ok) *ok = 1;
    return numeric_from_f64(d);
}

/* Convenience wrapper that ignores the ok flag. */
NUMERIC_ONLY_INLINE Numeric numeric_from_cstr(const char* s) {
    int ok_dummy;
    return numeric_from_string(s, &ok_dummy);
}

/* ================= Conversions and comparison ===================== */
/* Convert any Numeric to double (best-effort). Returns 0 on success. */
NUMERIC_ONLY_INLINE int numeric_to_f64(const Numeric* n, double* out) {
    if (!n || !out) return -1;
    switch (n->tag) {
        case NUMERIC_I8:   *out = (double)n->as.i8;   return 0;
        case NUMERIC_U8:   *out = (double)n->as.u8;   return 0;
        case NUMERIC_I16:  *out = (double)n->as.i16;  return 0;
        case NUMERIC_U16:  *out = (double)n->as.u16;  return 0;
        case NUMERIC_I32:  *out = (double)n->as.i32;  return 0;
        case NUMERIC_U32:  *out = (double)n->as.u32;  return 0;
        case NUMERIC_I64:  *out = (double)n->as.i64;  return 0;
        case NUMERIC_U64:  *out = (double)n->as.u64;  return 0;

        case NUMERIC_INT:    *out = (double)n->as.i;    return 0;
        case NUMERIC_UINT:   *out = (double)n->as.ui;   return 0;
        case NUMERIC_LONG:   *out = (double)n->as.l;    return 0;
        case NUMERIC_ULONG:  *out = (double)n->as.ul;   return 0;
        case NUMERIC_LLONG:  *out = (double)n->as.ll;   return 0;
        case NUMERIC_ULLONG: *out = (double)n->as.ull;  return 0;

        case NUMERIC_INTPTR: *out = (double)n->as.ip;   return 0;
        case NUMERIC_UINTPTR:*out = (double)n->as.uip;  return 0;
        case NUMERIC_SIZE:   *out = (double)n->as.zs;   return 0;
        case NUMERIC_PTRDIFF:*out = (double)n->as.pd;   return 0;
        case NUMERIC_IMAX:   *out = (double)n->as.im;   return 0;
        case NUMERIC_UMAX:   *out = (double)n->as.um;   return 0;

        case NUMERIC_F32:    *out = (double)n->as.f32;  return 0;
        case NUMERIC_F64:    *out = n->as.f64;          return 0;
        case NUMERIC_F128:   *out = (double)n->as.f128; return 0;

#if NUMERIC_ONLY_HAVE_INT128
        case NUMERIC_I128:   *out = (double)n->as.i128; return 0;
        case NUMERIC_U128:   *out = (double)n->as.u128; return 0;
#endif
        default: return -1;
    }
}

/* Compare two Numeric values via long double. Returns -1, 0, or +1. */
NUMERIC_ONLY_INLINE int numeric_compare(const Numeric* a, const Numeric* b) {
    if (!a || !b) return 0;

    long double av = 0.0L, bv = 0.0L;

    /* a -> long double */
    switch (a->tag) {
        case NUMERIC_I8:   av = (long double)a->as.i8;   break;
        case NUMERIC_U8:   av = (long double)a->as.u8;   break;
        case NUMERIC_I16:  av = (long double)a->as.i16;  break;
        case NUMERIC_U16:  av = (long double)a->as.u16;  break;
        case NUMERIC_I32:  av = (long double)a->as.i32;  break;
        case NUMERIC_U32:  av = (long double)a->as.u32;  break;
        case NUMERIC_I64:  av = (long double)a->as.i64;  break;
        case NUMERIC_U64:  av = (long double)a->as.u64;  break;

        case NUMERIC_INT:    av = (long double)a->as.i;    break;
        case NUMERIC_UINT:   av = (long double)a->as.ui;   break;
        case NUMERIC_LONG:   av = (long double)a->as.l;    break;
        case NUMERIC_ULONG:  av = (long double)a->as.ul;   break;
        case NUMERIC_LLONG:  av = (long double)a->as.ll;   break;
        case NUMERIC_ULLONG: av = (long double)a->as.ull;  break;

        case NUMERIC_INTPTR: av = (long double)a->as.ip;   break;
        case NUMERIC_UINTPTR:av = (long double)a->as.uip;  break;
        case NUMERIC_SIZE:   av = (long double)a->as.zs;   break;
        case NUMERIC_PTRDIFF:av = (long double)a->as.pd;   break;
        case NUMERIC_IMAX:   av = (long double)a->as.im;   break;
        case NUMERIC_UMAX:   av = (long double)a->as.um;   break;

        case NUMERIC_F32:    av = (long double)a->as.f32;  break;
        case NUMERIC_F64:    av = (long double)a->as.f64;  break;
        case NUMERIC_F128:   av = a->as.f128;              break;

#if NUMERIC_ONLY_HAVE_INT128
        case NUMERIC_I128:   av = (long double)a->as.i128; break;
        case NUMERIC_U128:   av = (long double)a->as.u128; break;
#endif
        default: break;
    }

    /* b -> long double */
    switch (b->tag) {
        case NUMERIC_I8:   bv = (long double)b->as.i8;   break;
        case NUMERIC_U8:   bv = (long double)b->as.u8;   break;
        case NUMERIC_I16:  bv = (long double)b->as.i16;  break;
        case NUMERIC_U16:  bv = (long double)b->as.u16;  break;
        case NUMERIC_I32:  bv = (long double)b->as.i32;  break;
        case NUMERIC_U32:  bv = (long double)b->as.u32;  break;
        case NUMERIC_I64:  bv = (long double)b->as.i64;  break;
        case NUMERIC_U64:  bv = (long double)b->as.u64;  break;

        case NUMERIC_INT:    bv = (long double)b->as.i;    break;
        case NUMERIC_UINT:   bv = (long double)b->as.ui;   break;
        case NUMERIC_LONG:   bv = (long double)b->as.l;    break;
        case NUMERIC_ULONG:  bv = (long double)b->as.ul;   break;
        case NUMERIC_LLONG:  bv = (long double)b->as.ll;   break;
        case NUMERIC_ULLONG: bv = (long double)b->as.ull;  break;

        case NUMERIC_INTPTR: bv = (long double)b->as.ip;   break;
        case NUMERIC_UINTPTR:bv = (long double)b->as.uip;  break;
        case NUMERIC_SIZE:   bv = (long double)b->as.zs;   break;
        case NUMERIC_PTRDIFF:bv = (long double)b->as.pd;   break;
        case NUMERIC_IMAX:   bv = (long double)b->as.im;   break;
        case NUMERIC_UMAX:   bv = (long double)b->as.um;   break;

        case NUMERIC_F32:    bv = (long double)b->as.f32;  break;
        case NUMERIC_F64:    bv = (long double)b->as.f64;  break;
        case NUMERIC_F128:   bv = b->as.f128;              break;

#if NUMERIC_ONLY_HAVE_INT128
        case NUMERIC_I128:   bv = (long double)b->as.i128; break;
        case NUMERIC_U128:   bv = (long double)b->as.u128; break;
#endif
        default: break;
    }

    if (av < bv) return -1;
    if (av > bv) return +1;
    return 0;
}

/* ==================== "Any pointer" -> Numeric ===================== */
/* Typed fallback API (works on all compilers, including pre-C11):
   - value must point to an object of the type described by tag, OR
   - pass a pointer to C-string and use tag NUMERIC_F64_STRING_INPUT.
*/
enum { NUMERIC_F64_STRING_INPUT = 10000 }; /* sentinel tag for C-strings */

NUMERIC_ONLY_INLINE Numeric numeric_from_value_typed(const void* value, int tag_or_string, int* ok) {
    if (ok) *ok = 0;
    if (!value) return numeric_from_f64(NAN);

    if (tag_or_string == NUMERIC_F64_STRING_INPUT) {
        return numeric_from_string((const char*)value, ok);
    }

    switch ((NumericTag)tag_or_string) {
        case NUMERIC_I8:    if (ok) *ok=1; return numeric_from_i8   (*(const int8_t*)value);
        case NUMERIC_U8:    if (ok) *ok=1; return numeric_from_u8   (*(const uint8_t*)value);
        case NUMERIC_I16:   if (ok) *ok=1; return numeric_from_i16  (*(const int16_t*)value);
        case NUMERIC_U16:   if (ok) *ok=1; return numeric_from_u16  (*(const uint16_t*)value);
        case NUMERIC_I32:   if (ok) *ok=1; return numeric_from_i32  (*(const int32_t*)value);
        case NUMERIC_U32:   if (ok) *ok=1; return numeric_from_u32  (*(const uint32_t*)value);
        case NUMERIC_I64:   if (ok) *ok=1; return numeric_from_i64  (*(const int64_t*)value);
        case NUMERIC_U64:   if (ok) *ok=1; return numeric_from_u64  (*(const uint64_t*)value);

        case NUMERIC_INT:   if (ok) *ok=1; return numeric_from_int  (*(const int*)value);
        case NUMERIC_UINT:  if (ok) *ok=1; return numeric_from_uint (*(const unsigned*)value);
        case NUMERIC_LONG:  if (ok) *ok=1; return numeric_from_long (*(const long*)value);
        case NUMERIC_ULONG: if (ok) *ok=1; return numeric_from_ulong(*(const unsigned long*)value);
        case NUMERIC_LLONG: if (ok) *ok=1; return numeric_from_llong(*(const long long*)value);
        case NUMERIC_ULLONG:if (ok) *ok=1; return numeric_from_ullong(*(const unsigned long long*)value);

        case NUMERIC_INTPTR: if (ok) *ok=1; return numeric_from_intptr (*(const intptr_t*)value);
        case NUMERIC_UINTPTR:if (ok) *ok=1; return numeric_from_uintptr(*(const uintptr_t*)value);
        case NUMERIC_SIZE:   if (ok) *ok=1; return numeric_from_size   (*(const size_t*)value);
        case NUMERIC_PTRDIFF:if (ok) *ok=1; return numeric_from_ptrdiff(*(const ptrdiff_t*)value);
        case NUMERIC_IMAX:   if (ok) *ok=1; return numeric_from_imax   (*(const intmax_t*)value);
        case NUMERIC_UMAX:   if (ok) *ok=1; return numeric_from_umax   (*(const uintmax_t*)value);

        case NUMERIC_F32:   if (ok) *ok=1; return numeric_from_f32 (*(const float*)value);
        case NUMERIC_F64:   if (ok) *ok=1; return numeric_from_f64 (*(const double*)value);
        case NUMERIC_F128:  if (ok) *ok=1; return numeric_from_f128(*(const long double*)value);

#if NUMERIC_ONLY_HAVE_INT128
        case NUMERIC_I128:  if (ok) *ok=1; return numeric_from_i128(*(const numeric_only_int128_t*)value);
        case NUMERIC_U128:  if (ok) *ok=1; return numeric_from_u128(*(const numeric_only_uint128_t*)value);
#endif
        default: break;
    }
    return numeric_from_f64(NAN);
}

/* C11 _Generic convenience: numeric_from_value(ptr, &ok)
   - If ptr is char* / const char*      -> parse string.
   - If ptr is pointer to numeric type  -> dereference and wrap.
   - Otherwise                          -> returns NaN and sets *ok=0.
*/
#if NUMERIC_ONLY_HAVE_GENERIC

/* Helpers to set ok and wrap pointers (kept small and inlined). */
NUMERIC_ONLY_INLINE Numeric __no_from_unsupported(const void* p, int* ok) {
    (void)p; if (ok) *ok = 0; return numeric_from_f64(NAN);
}
#define __NO_DEF_PTR_HELPER(T, FN) \
    NUMERIC_ONLY_INLINE Numeric FN(const T* p, int* ok){ if(ok)*ok=1; return numeric_from_##FN == numeric_from_##FN ? numeric_from_##FN(*p) : numeric_from_##FN(*p); }
/* Note: The macro above is not actually used; we define explicit helpers below. */

/* Explicit helpers (clear and MSVC-friendly) */
NUMERIC_ONLY_INLINE Numeric __no_i8 (const int8_t* p, int* ok){ if(ok)*ok=1; return numeric_from_i8 (*p); }
NUMERIC_ONLY_INLINE Numeric __no_u8 (const uint8_t* p,int* ok){ if(ok)*ok=1; return numeric_from_u8 (*p); }
NUMERIC_ONLY_INLINE Numeric __no_i16(const int16_t* p,int* ok){ if(ok)*ok=1; return numeric_from_i16(*p); }
NUMERIC_ONLY_INLINE Numeric __no_u16(const uint16_t*p,int* ok){ if(ok)*ok=1; return numeric_from_u16(*p); }
NUMERIC_ONLY_INLINE Numeric __no_i32(const int32_t* p,int* ok){ if(ok)*ok=1; return numeric_from_i32(*p); }
NUMERIC_ONLY_INLINE Numeric __no_u32(const uint32_t*p,int* ok){ if(ok)*ok=1; return numeric_from_u32(*p); }
NUMERIC_ONLY_INLINE Numeric __no_i64(const int64_t* p,int* ok){ if(ok)*ok=1; return numeric_from_i64(*p); }
NUMERIC_ONLY_INLINE Numeric __no_u64(const uint64_t*p,int* ok){ if(ok)*ok=1; return numeric_from_u64(*p); }

NUMERIC_ONLY_INLINE Numeric __no_int (const int* p, int* ok){ if(ok)*ok=1; return numeric_from_int (*p); }
NUMERIC_ONLY_INLINE Numeric __no_uint(const unsigned* p,int* ok){ if(ok)*ok=1; return numeric_from_uint(*p); }
NUMERIC_ONLY_INLINE Numeric __no_long (const long* p,int* ok){ if(ok)*ok=1; return numeric_from_long (*p); }
NUMERIC_ONLY_INLINE Numeric __no_ulong(const unsigned long* p,int* ok){ if(ok)*ok=1; return numeric_from_ulong(*p); }
NUMERIC_ONLY_INLINE Numeric __no_llong(const long long* p,int* ok){ if(ok)*ok=1; return numeric_from_llong(*p); }
NUMERIC_ONLY_INLINE Numeric __no_ullong(const unsigned long long* p,int* ok){ if(ok)*ok=1; return numeric_from_ullong(*p); }

NUMERIC_ONLY_INLINE Numeric __no_ip (const intptr_t* p, int* ok){ if(ok)*ok=1; return numeric_from_intptr (*p); }
NUMERIC_ONLY_INLINE Numeric __no_uip(const uintptr_t* p,int* ok){ if(ok)*ok=1; return numeric_from_uintptr(*p); }
NUMERIC_ONLY_INLINE Numeric __no_size(const size_t* p, int* ok){ if(ok)*ok=1; return numeric_from_size   (*p); }
NUMERIC_ONLY_INLINE Numeric __no_pd  (const ptrdiff_t* p,int* ok){ if(ok)*ok=1; return numeric_from_ptrdiff(*p); }
NUMERIC_ONLY_INLINE Numeric __no_imax(const intmax_t* p,int* ok){ if(ok)*ok=1; return numeric_from_imax  (*p); }
NUMERIC_ONLY_INLINE Numeric __no_umax(const uintmax_t* p,int* ok){ if(ok)*ok=1; return numeric_from_umax  (*p); }

NUMERIC_ONLY_INLINE Numeric __no_f32 (const float* p, int* ok){ if(ok)*ok=1; return numeric_from_f32 (*p); }
NUMERIC_ONLY_INLINE Numeric __no_f64 (const double* p,int* ok){ if(ok)*ok=1; return numeric_from_f64 (*p); }
NUMERIC_ONLY_INLINE Numeric __no_f128(const long double* p,int* ok){ if(ok)*ok=1; return numeric_from_f128(*p); }

#if NUMERIC_ONLY_HAVE_INT128
NUMERIC_ONLY_INLINE Numeric __no_i128(const numeric_only_int128_t* p, int* ok){ if(ok)*ok=1; return numeric_from_i128(*p); }
NUMERIC_ONLY_INLINE Numeric __no_u128(const numeric_only_uint128_t* p,int* ok){ if(ok)*ok=1; return numeric_from_u128(*p); }
#endif

/* The _Generic-based convenience macro */
#define numeric_from_value(VPTR, OKPTR) \
  _Generic((VPTR), \
    /* C-strings -> parse */ \
    const char*:        numeric_from_string((const char*)(VPTR), (OKPTR)), \
    char*:              numeric_from_string((const char*)(VPTR), (OKPTR)), \
    /* Fixed-width */ \
    const int8_t*:      __no_i8  ((const int8_t*)(VPTR),  (OKPTR)), \
    int8_t*:            __no_i8  ((const int8_t*)(VPTR),  (OKPTR)), \
    const uint8_t*:     __no_u8  ((const uint8_t*)(VPTR), (OKPTR)), \
    uint8_t*:           __no_u8  ((const uint8_t*)(VPTR), (OKPTR)), \
    const int16_t*:     __no_i16 ((const int16_t*)(VPTR), (OKPTR)), \
    int16_t*:           __no_i16 ((const int16_t*)(VPTR), (OKPTR)), \
    const uint16_t*:    __no_u16 ((const uint16_t*)(VPTR),(OKPTR)), \
    uint16_t*:          __no_u16 ((const uint16_t*)(VPTR),(OKPTR)), \
    const int32_t*:     __no_i32 ((const int32_t*)(VPTR), (OKPTR)), \
    int32_t*:           __no_i32 ((const int32_t*)(VPTR), (OKPTR)), \
    const uint32_t*:    __no_u32 ((const uint32_t*)(VPTR),(OKPTR)), \
    uint32_t*:          __no_u32 ((const uint32_t*)(VPTR),(OKPTR)), \
    const int64_t*:     __no_i64 ((const int64_t*)(VPTR), (OKPTR)), \
    int64_t*:           __no_i64 ((const int64_t*)(VPTR), (OKPTR)), \
    const uint64_t*:    __no_u64 ((const uint64_t*)(VPTR),(OKPTR)), \
    uint64_t*:          __no_u64 ((const uint64_t*)(VPTR),(OKPTR)), \
    /* Native families */ \
    const int*:         __no_int ((const int*)(VPTR),          (OKPTR)), \
    int*:               __no_int ((const int*)(VPTR),          (OKPTR)), \
    const unsigned*:    __no_uint((const unsigned*)(VPTR),     (OKPTR)), \
    unsigned*:          __no_uint((const unsigned*)(VPTR),     (OKPTR)), \
    const long*:        __no_long((const long*)(VPTR),         (OKPTR)), \
    long*:              __no_long((const long*)(VPTR),         (OKPTR)), \
    const unsigned long*: __no_ulong((const unsigned long*)(VPTR),(OKPTR)), \
    unsigned long*:       __no_ulong((const unsigned long*)(VPTR),(OKPTR)), \
    const long long*:   __no_llong((const long long*)(VPTR),   (OKPTR)), \
    long long*:         __no_llong((const long long*)(VPTR),   (OKPTR)), \
    const unsigned long long*: __no_ullong((const unsigned long long*)(VPTR),(OKPTR)), \
    unsigned long long*:       __no_ullong((const unsigned long long*)(VPTR),(OKPTR)), \
    /* Pointer-sized / max-width */ \
    const intptr_t*:    __no_ip  ((const intptr_t*)(VPTR),     (OKPTR)), \
    intptr_t*:          __no_ip  ((const intptr_t*)(VPTR),     (OKPTR)), \
    const uintptr_t*:   __no_uip ((const uintptr_t*)(VPTR),    (OKPTR)), \
    uintptr_t*:         __no_uip ((const uintptr_t*)(VPTR),    (OKPTR)), \
    const size_t*:      __no_size((const size_t*)(VPTR),       (OKPTR)), \
    size_t*:            __no_size((const size_t*)(VPTR),       (OKPTR)), \
    const ptrdiff_t*:   __no_pd  ((const ptrdiff_t*)(VPTR),    (OKPTR)), \
    ptrdiff_t*:         __no_pd  ((const ptrdiff_t*)(VPTR),    (OKPTR)), \
    const intmax_t*:    __no_imax((const intmax_t*)(VPTR),     (OKPTR)), \
    intmax_t*:          __no_imax((const intmax_t*)(VPTR),     (OKPTR)), \
    const uintmax_t*:   __no_umax((const uintmax_t*)(VPTR),    (OKPTR)), \
    uintmax_t*:         __no_umax((const uintmax_t*)(VPTR),    (OKPTR)), \
    /* Floating point */ \
    const float*:       __no_f32 ((const float*)(VPTR),        (OKPTR)), \
    float*:             __no_f32 ((const float*)(VPTR),        (OKPTR)), \
    const double*:      __no_f64 ((const double*)(VPTR),       (OKPTR)), \
    double*:            __no_f64 ((const double*)(VPTR),       (OKPTR)), \
    const long double*: __no_f128((const long double*)(VPTR),  (OKPTR)), \
    long double*:       __no_f128((const long double*)(VPTR),  (OKPTR)) \
    /* default: unsupported pointer type -> NaN */ \
  , default: __no_from_unsupported((const void*)(VPTR), (OKPTR)) )
#endif /* NUMERIC_ONLY_HAVE_GENERIC */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NUMERIC_ONLY_H */
