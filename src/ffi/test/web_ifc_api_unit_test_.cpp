#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

static void test_set_log_level_and_version_and_encoding(void) {
    IfcAPI *api = ifc_api_new();
    ASSERT(api != NULL, "ifc_api_new returned NULL");
    ifc_api_init(api);

    /* Set log level - should be safe */
    ifc_api_set_log_level(api, LOG_LEVEL_DEBUG);

    /* Get version */
    size_t need = ifc_api_get_version(api, nullptr);
    ASSERT(need > 0, "version reported size is zero");
    std::string ver; ver.resize(need);
    size_t written = ifc_api_get_version(api, ver.data());
    ASSERT(written == need, "version written size mismatch");

    /* Encode / decode text roundtrip */
    const char *text = "Hello \"IFC\" \X2\\03C0\\X0\\"; /* contains pi as encoded */
    size_t enc_need = ifc_api_encode_text(api, text, nullptr);
    ASSERT(enc_need > 0, "encode_text need zero");
    std::string enc; enc.resize(enc_need);
    size_t enc_written = ifc_api_encode_text(api, text, enc.data());
    ASSERT(enc_written == enc_need, "encode_text write mismatch");

    size_t dec_need = ifc_api_decode_text(api, enc.c_str(), nullptr);
    ASSERT(dec_need > 0, "decode_text need zero");
    std::string dec; dec.resize(dec_need);
    size_t dec_written = ifc_api_decode_text(api, enc.c_str(), dec.data());
    ASSERT(dec_written == dec_need, "decode_text write mismatch");
    /* decoded should contain original-ish text (we check substring) */
    ASSERT(dec.find("Hello") != std::string::npos, "decoded text missing Hello");

    ifc_api_free(api);
}

static void test_open_model_invalid_and_all_lines(void) {
    IfcAPI *api = ifc_api_new();
    ASSERT(api != NULL, "ifc_api_new returned NULL");
    ifc_api_init(api);

    /* empty buffer should return -1 */
    ByteArray empty = { NULL, 0 };
    int id = ifc_api_open_model(api, empty, NULL);
    ASSERT(id == -1, "ifc_api_open_model should return -1 for empty buffer");

    /* calling get_all_lines on invalid model should return 0 */
    size_t bytes = ifc_api_get_all_lines(api, 0, nullptr);
    ASSERT(bytes == 0, "get_all_lines on invalid model should be 0");

    ifc_api_free(api);
}

static std::string load_example_ifc_file_contents()
{
    // Try a set of relative paths depending on execution working directory
    const char* candidates[] = {
        "../../tests/ifcfiles/public/example.ifc",
        "../../../tests/ifcfiles/public/example.ifc",
        "../tests/ifcfiles/public/example.ifc",
        "tests/ifcfiles/public/example.ifc",
        "../../tests/ifcfiles/public/AC20-FZK-Haus.ifc"
    };
    for (const char* c : candidates) {
        FILE *f = fopen(c, "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz <= 0) { fclose(f); continue; }
        std::string data;
        data.resize(sz);
        size_t rd = fread(&data[0], 1, sz, f);
        fclose(f);
        if ((long)rd == sz) return data;
    }
    return {};
}

static void test_open_example_ifc_and_basic_queries(void) {
    std::string data = load_example_ifc_file_contents();
    if (data.empty()) {
        /* Skip test if example file not found in CI environment */
        printf("Skipping example.ifc based tests (file not found).\n");
        return;
    }

    IfcAPI *api = ifc_api_new();
    ASSERT(api != NULL, "ifc_api_new returned NULL");
    ifc_api_init(api);

    ByteArray ba;
    ba.data = (uint8_t *)data.data();
    ba.len = data.size();

    int modelId = ifc_api_open_model(api, ba, NULL);
    ASSERT(modelId >= 0, "ifc_api_open_model failed for example.ifc");
    ASSERT(ifc_api_is_model_open(api, (uint32_t)modelId), "model should be open");

    /* get max express id should be > 0 */
    uint32_t maxId = ifc_api_get_max_express_id(api, (uint32_t)modelId);
    ASSERT(maxId > 0, "max express id should be > 0");

    /* get model schema string */
    size_t need = ifc_api_get_model_schema(api, (uint32_t)modelId, nullptr);
    ASSERT(need > 0, "model_schema need should be > 0");
    std::string schema; schema.resize(need);
    size_t written = ifc_api_get_model_schema(api, (uint32_t)modelId, schema.data());
    ASSERT(written == need, "model_schema written mismatch");
    ASSERT(schema.size() > 0, "schema string empty");

    /* try coordination matrix preflight/write */
    size_t coord_need = ifc_api_get_coordination_matrix(api, (uint32_t)modelId, nullptr);
    ASSERT(coord_need == sizeof(double) * 16, "coordination matrix preflight should be 16 doubles");
    std::vector<double> mat(16);
    size_t coord_written = ifc_api_get_coordination_matrix(api, (uint32_t)modelId, mat.data());
    ASSERT(coord_written == coord_need, "coordination matrix written mismatch");

    /* close model */
    ifc_api_close_model(api, (uint32_t)modelId);
    ASSERT(!ifc_api_is_model_open(api, (uint32_t)modelId), "model should be closed");

    ifc_api_free(api);
}

int main(void) {
    test_new_init_free();
    test_free_null();
    test_set_log_level_and_version_and_encoding();
    test_open_model_invalid_and_all_lines();
    test_open_example_ifc_and_basic_queries();

    printf("All web_ifc_api unit tests passed.\n");
    return 0;
}
