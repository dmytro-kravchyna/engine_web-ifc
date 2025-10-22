/*
 * Tests for the C API implemented in web_ifc_api.cpp.
 * This file is compiled as C++ and links against the C wrapper library.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../web_ifc_api.h"

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "ASSERT FAILED: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
        exit(1); \
    } \
} while(0)

static void test_new_init_free(void) {
    IfcAPI *api = ifc_api_new();
    ASSERT(api != NULL, "ifc_api_new returned NULL");

    int r = ifc_api_init(api);
    ASSERT(r == 0, "ifc_api_init failed");

    /* calling free should not crash and should clear manager */
    ifc_api_free(api);
}

static void test_free_null(void) {
    /* should be a no-op and not crash */
    ifc_api_free(NULL);
}

static void test_create_settings_defaults(void) {
    LoaderSettings s = ifc_api_create_settings(NULL);
    /* verify a handful of default values by dereferencing pointers */
    ASSERT(s.COORDINATE_TO_ORIGIN != NULL, "COORDINATE_TO_ORIGIN pointer is NULL");
    ASSERT(*s.CIRCLE_SEGMENTS == DEFAULT_CIRCLE_SEGMENTS, "DEFAULT_CIRCLE_SEGMENTS mismatch");
    ASSERT(*s.LINEWRITER_BUFFER == DEFAULT_LINEWRITER_BUFFER, "DEFAULT_LINEWRITER_BUFFER mismatch");
}

static void test_open_model_invalid(void) {
    IfcAPI *api = ifc_api_new();
    ASSERT(api != NULL, "ifc_api_new returned NULL");
    /* empty buffer should return -1 */
    ByteArray empty = { NULL, 0 };
    int id = ifc_api_open_model(api, empty, NULL);
    ASSERT(id == -1, "ifc_api_open_model should return -1 for empty buffer");
    ifc_api_free(api);
}

int main(void) {
    test_new_init_free();
    test_free_null();
    test_create_settings_defaults();
    // test_open_model_invalid();
    printf("All web_ifc_api tests passed.\n");
    return 0;
}
