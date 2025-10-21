/* numeric_only.h
   Portable numeric-only tagged union with wide type coverage and string parsing.
   Platforms: iOS, Android (NDK), Windows (MSVC), Linux, macOS.
   Language: C99 or newer (works in C++ via extern "C").
*/

#ifndef NUMERIC_ONLY_H
#define NUMERIC_ONLY_H

/* ----- Standard headers (portable) ----- */
#include <stdint.h>   /* int*_t, uint*_t, intptr_t, uintptr_t, intmax_t, uintmax_t */
#include <stddef.h>   /* size_t, ptrdiff_t */
#include <stdlib.h>   /* strtod */
#include <errno.h>    /* errno, ERANGE */
#include <math.h>     /* NAN (fallback provided below if missing) */

#ifdef __cplusplus
extern "C" {
#endif

/* ----- Fallback for NAN on very old C libraries ----- */
#ifndef NAN
  /* As a last resort, define a quiet NaN through 0.0/0.0.
     This may trigger a compile-time warning on some toolchains, but is portable. */
  #define NAN (0.0/0.0)
#endif

/* ----- Optional 128-bit integer support (GCC/Clang; typically not MSVC) ----- */
/* You can force-disable with: #define NUMERIC_ONLY_DISABLE_INT128 1 before including. */
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

/* ----- API visibility/inline (customize if embedding in a DLL) ----- */
#ifndef NUMERIC_ONLY_INLINE
  #define NUMERIC_ONLY_INLINE static inline
#endif

/* ----- Tag describing which variant is stored ----- */
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
    /* Optional 128-bit integers (GCC/Clang) */
    NUMERIC_I128, NUMERIC_U128,
#endif
} NumericTag;

/* ----- Numeric-only tagged union (no strings, no external pointers) ----- */
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

/* ======================== String parsing (double) ===================== */
/* Parses a C string with strtod (locale-dependent).
   - If s is NULL or parsing fails/overflows, returns NUMERIC_F64 with NaN.
   - On success, returns NUMERIC_F64 with the parsed value.
   - If ok!=NULL, *ok is set to 1 on success, 0 on failure.
*/
NUMERIC_ONLY_INLINE Numeric numeric_from_string(const char* s, int* ok) {
    if (ok) *ok = 0;
    if (!s) {
        return numeric_from_f64(NAN);
    }
    errno = 0;
    char* endp = NULL;
    double d = strtod(s, &endp);
    if (endp == s || errno == ERANGE) {
        return numeric_from_f64(NAN);
    }
    if (ok) *ok = 1;
    return numeric_from_f64(d);
}

/* ===================== Conversions and comparison ==================== */
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
        case NUMERIC_I128:   *out = (double)n->as.i128; return 0; /* precision loss possible */
        case NUMERIC_U128:   *out = (double)n->as.u128; return 0; /* precision loss possible */
#endif
        default: return -1;
    }
}

/* Compare two Numeric values via long double. Returns -1, 0, or +1. */
NUMERIC_ONLY_INLINE int numeric_compare(const Numeric* a, const Numeric* b) {
    if (!a || !b) return 0;

    long double av = 0.0L, bv = 0.0L;

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NUMERIC_ONLY_H */
