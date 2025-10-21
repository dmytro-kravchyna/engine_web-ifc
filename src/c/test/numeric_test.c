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
        return 0;
    } else {
        printf("%d test(s) failed.\n", failures);
        return 2;
    }
}
