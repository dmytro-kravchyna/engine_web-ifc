// Comprehensive tests for generated ifc-schema.h (C++ version)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>

// Include the wrapper header.  In C++ mode this will resolve to either
// the generated C schema or the C++ schema from the WebIFC project
// depending on availability.  Compiling as C++ avoids duplicate
// enumerator definitions.
#include "ifc_schema.h"

static int assertions = 0;
static int failures = 0;

#define ASSERT_TRUE(cond, msg) do { \
  assertions++; \
  if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
} while(0)

// Verify the row/column metadata matches and indexes are sane
static void test_rows_and_indexes(void) {
  // Ensure we have at least one schema row
  ASSERT_TRUE((int)SCHEMA_NAME_ROWS >= 1, "SCHEMA_NAME_ROWS < 1");

  for (size_t r = 0; r < SCHEMA_NAME_ROWS; ++r) {
    uint32_t off = (uint32_t)SCHEMA_NAME_INDEX[r].off;
    uint32_t len = (uint32_t)SCHEMA_NAME_INDEX[r].len;
    ASSERT_TRUE(len >= 1, "row length < 1");
    ASSERT_TRUE(off + len <= (uint32_t)(sizeof(SCHEMA_NAME_DATA)/sizeof(SCHEMA_NAME_DATA[0])), "index out of bounds");
  }
}

// Verify get_schema_name returns non-empty strings for every cell
static void test_schema_name_values(void) {
  for (size_t r = 0; r < SCHEMA_NAME_ROWS; ++r) {
    for (size_t c = 0; c < SCHEMA_NAME_INDEX[r].len; ++c) {
      const char *s = get_schema_name((size_t)r, (size_t)c);
      char msg[128];
      snprintf(msg, sizeof(msg), "get_schema_name(%zu,%zu) returned NULL", r, c);
      ASSERT_TRUE(s != NULL, msg);
      if (s) {
        snprintf(msg, sizeof(msg), "get_schema_name(%zu,%zu) returned empty string", r, c);
        ASSERT_TRUE(strlen(s) > 0, msg);
      }
    }
  }
}

// Verify some generated macros exist and have usable values
static void test_macro_existence(void) {
  // These macros are generated; check they compile and are nonzero where expected
  ASSERT_TRUE(FILE_SCHEMA != 0, "FILE_SCHEMA == 0");
  // Ensure at least one IFC macro exists (IFC2X3 typically present)
  (void)IFC2X3;
  ASSERT_TRUE(1, "IFC2X3 macro referenced");
}

int main(void) {
  printf("Running ifc_schema tests...\n");
  test_rows_and_indexes();
  test_schema_name_values();
  test_macro_existence();

  if (failures == 0) {
    printf("All ifc_schema tests passed. (%d assertions)\n", assertions);
    return 0;
  } else {
    fprintf(stderr, "%d tests failed (out of %d assertions)\n", failures, assertions);
    return 1;
  }
}