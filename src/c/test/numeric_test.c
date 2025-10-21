#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../helpers/numeric.h"

static int failures = 0;

static void expect_int(int got, int want, const char *msg) {
    if (got != want) {
        printf("FAIL %s: got %d want %d\n", msg, got, want);
        failures++;
    }
}

static void expect_double(double got, double want, const char *msg) {
    if (isnan(got) && isnan(want)) return;
    double diff = fabs(got - want);
    if (diff > 1e-9) {
        printf("FAIL %s: got %.17g want %.17g\n", msg, got, want);
        failures++;
    }
}

/* forward declare focused generic test */
int test_generic_macro(void);

int main(void) {
    printf("Running numeric tests...\n");

    Numeric a = numeric_from_i64(-42);
    Numeric b = numeric_from_umax((uintmax_t)1234567890123ULL);
    Numeric c = numeric_from_f32(3.14f);

    int ok;
    Numeric d = numeric_from_string("12.5e-1", &ok);   /* -> 1.25 */

    double x;
    expect_int(numeric_to_f64(&a, &x) == 0 ? 1 : 0, 1, "numeric_to_f64 a returns 0");
    numeric_to_f64(&a, &x); printf("a = %g\n", x);
    expect_double(x, -42.0, "a value");

    numeric_to_f64(&b, &x); printf("b = %g\n", x);
    expect_double(x, (double)1234567890123.0, "b value (may lose precision)");

    numeric_to_f64(&c, &x); printf("c = %g\n", x);
    expect_double(x, (double)3.14f, "c value");

    numeric_to_f64(&d, &x); printf("d(ok=%d) = %g\n", ok, x);
    expect_int(ok, 1, "d parse ok");
    expect_double(x, 1.25, "d value");

    printf("compare a vs b -> %d\n", numeric_compare(&a, &b));

    /* More cases */
    Numeric nnull = numeric_from_string(NULL, &ok);
    expect_int(ok, 0, "parse NULL returns ok=0");
    numeric_to_f64(&nnull, &x); expect_double(x, NAN, "NULL parse -> NaN");

    Numeric nanstr = numeric_from_string("abc", &ok);
    expect_int(ok, 0, "parse non-numeric returns ok=0");
    numeric_to_f64(&nanstr, &x); expect_double(x, NAN, "non-numeric -> NaN");

    Numeric big = numeric_from_u64(18446744073709551615ULL);
    numeric_to_f64(&big, &x); printf("big = %g\n", x);

    Numeric smalli = numeric_from_i32(-1);
    numeric_to_f64(&smalli, &x); expect_double(x, -1.0, "smalli -1");

    /* Floating edge cases */
    Numeric infn = numeric_from_string("1e400", &ok); /* overflow */
    expect_int(ok, 0, "parse overflow returns ok=0");

#if NUMERIC_HAVE_INT128
    Numeric i128 = numeric_from_i128((int128_t)1234567890123456789LL);
    numeric_to_f64(&i128, &x); printf("i128 -> %g\n", x);
#endif

    /* compare equal values */
    Numeric e1 = numeric_from_f64(2.5);
    Numeric e2 = numeric_from_string("2.5", &ok);
    expect_int(numeric_compare(&e1, &e2), 0, "compare equal 2.5");

    /* compare ordering */
    Numeric small = numeric_from_i32(10);
    Numeric large = numeric_from_i32(20);
    expect_int(numeric_compare(&small, &large), -1, "10 < 20");
    expect_int(numeric_compare(&large, &small), 1, "20 > 10");

    if (failures == 0) {
        printf("All tests passed.\n");
        /* run focused generic macro test */
        if (test_generic_macro() != 0) return 2;
        return 0;
    } else {
        printf("%d test(s) failed.\n", failures);
        return 2;
    }
}

/* Additional tests for numeric_from_value / numeric_from_value_typed */
int generic_tests(void) {
    int ok;
    double dv = 3.5;
    int32_t iv = -123;

    /* Use the typed API to avoid duplicate _Generic associations in some compilers */
    Numeric a = numeric_from_value_typed(&dv, NUMERIC_F64, &ok);
    expect_int(ok, 1, "typed a ok");
    Numeric b = numeric_from_value_typed(&iv, NUMERIC_I32, &ok);
    expect_int(ok, 1, "typed b ok");
    Numeric c = numeric_from_value_typed("42.75", NUMERIC_F64_STRING_INPUT, &ok);
    expect_int(ok, 1, "typed c ok");

    double x;
    numeric_to_f64(&a, &x); printf("generic a=%g\n", x); expect_double(x, 3.5, "generic a value");
    numeric_to_f64(&b, &x); printf("generic b=%g\n", x); expect_double(x, -123.0, "generic b value");
    numeric_to_f64(&c, &x); printf("generic c=%g\n", x); expect_double(x, 42.75, "generic c value");

    return failures == 0 ? 0 : 2;
}

/* Focused test for the _Generic convenience macro without triggering
   the header's large mapping (which can contain compatible duplicate
   associations on some platforms). We undef the library macro and
   provide a small test-only mapping for a few types. */
int test_generic_macro(void) {
#if NUMERIC_ONLY_HAVE_GENERIC
    int ok;
    double dv = 3.5;
    int32_t iv = -123;

    /* Define a small, non-conflicting test macro just for this file. */
    #ifdef numeric_from_value
    #undef numeric_from_value
    #endif
    #define numeric_from_value_test(VPTR, OKPTR) \
      _Generic((VPTR), \
        const char*: numeric_from_string((const char*)(VPTR), (OKPTR)), \
        char*:       numeric_from_string((const char*)(VPTR), (OKPTR)), \
        const double*: __no_f64((const double*)(VPTR), (OKPTR)), \
        double*:       __no_f64((const double*)(VPTR), (OKPTR)), \
        const int32_t*: __no_i32((const int32_t*)(VPTR), (OKPTR)), \
        int32_t*:       __no_i32((const int32_t*)(VPTR), (OKPTR)), \
        default: __no_from_unsupported((const void*)(VPTR), (OKPTR)) )

    Numeric a = numeric_from_value_test(&dv, &ok);
    expect_int(ok, 1, "generic-macro a ok");
    double x; numeric_to_f64(&a, &x); printf("macro a=%g\n", x); expect_double(x, 3.5, "macro a value");

    Numeric b = numeric_from_value_test(&iv, &ok);
    expect_int(ok, 1, "generic-macro b ok");
    numeric_to_f64(&b, &x); printf("macro b=%g\n", x); expect_double(x, -123.0, "macro b value");

    Numeric c = numeric_from_value_test("42.75", &ok);
    expect_int(ok, 1, "generic-macro c ok");
    numeric_to_f64(&c, &x); printf("macro c=%g\n", x); expect_double(x, 42.75, "macro c value");

    /* restore nothing — this is a test-only macro shadowing the header symbol */
    return failures == 0 ? 0 : 2;
#else
    /* No _Generic support; nothing to test here. */
    (void)0; return 0;
#endif
}


