// // Translated key tests from WebIfcApi.spec.ts into C++ tests calling the
// // C FFI defined in web_ifc_api.h. Tests avoid calling C API functions that
// // are currently unimplemented in the C wrapper and will skip assertions when
// // appropriate.

// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_test_macros.hpp>
// #include <catch2/catch_approx.hpp>

// #include "../web_ifc_api.h"

// #include <filesystem>
// #include <fstream>
// #include <string>
// #include <vector>

// static std::string load_example_ifc_contents()
// {
//     const char* candidates[] = {
//         "../../tests/ifcfiles/public/example.ifc",
//         "../../../tests/ifcfiles/public/example.ifc",
//         "../tests/ifcfiles/public/example.ifc",
//         "tests/ifcfiles/public/example.ifc",
//         "../../tests/ifcfiles/public/AC20-FZK-Haus.ifc"
//     };
//     for (const char* c : candidates) {
//         if (std::filesystem::exists(std::filesystem::path(c))) {
//             std::ifstream f(c, std::ios::binary);
//             if (!f) continue;
//             f.seekg(0, std::ios::end);
//             std::streamsize sz = f.tellg();
//             f.seekg(0, std::ios::beg);
//             if (sz <= 0) continue;
//             std::string s; s.resize((size_t)sz);
//             f.read(&s[0], sz);
//             return s;
//         }
//     }
//     return {};
// }

// TEST_CASE("WebIfcApi C FFI basic reading methods", "[WebIfcApi][FFI]")
// {
//     IfcAPI *api = ifc_api_new();
//     REQUIRE(api != NULL);
//     REQUIRE(ifc_api_init(api) == 0);

//     // Silence logs for tests
//     ifc_api_set_log_level(api, LOG_LEVEL_OFF);

//     std::string content = load_example_ifc_contents();
//     if (content.empty()) {
//         WARN("No example.ifc found in test paths - skipping file-based tests");
//         ifc_api_free(api);
//         return;
//     }

//     ByteArray ba;
//     ba.data = (uint8_t*)content.data();
//     ba.len = content.size();

//     int32_t modelID = ifc_api_open_model(api, ba, NULL);
//     REQUIRE(modelID >= 0);
//     REQUIRE(ifc_api_is_model_open(api, (uint32_t)modelID));

//     SECTION("Get max express id") {
//         uint32_t maxId = ifc_api_get_max_express_id(api, (uint32_t)modelID);
//         REQUIRE(maxId > 0);
//     }

//     SECTION("Get model schema string") {
//         size_t need = ifc_api_get_model_schema(api, (uint32_t)modelID, nullptr);
//         REQUIRE(need > 0);
//         std::string schema; schema.resize(need);
//         size_t written = ifc_api_get_model_schema(api, (uint32_t)modelID, schema.data());
//         REQUIRE(written == need);
//         REQUIRE(schema.size() > 0);
//     }

//     SECTION("Get coordination matrix") {
//         size_t need = ifc_api_get_coordination_matrix(api, (uint32_t)modelID, nullptr);
//         REQUIRE(need == sizeof(double) * 16);
//         std::vector<double> mat(16);
//         size_t written = ifc_api_get_coordination_matrix(api, (uint32_t)modelID, mat.data());
//         REQUIRE(written == need);
//         // Expect identity on diagonal
//         REQUIRE(mat[0] == Catch::Approx(1.0));
//         REQUIRE(mat[5] == Catch::Approx(1.0));
//         REQUIRE(mat[10] == Catch::Approx(1.0));
//         REQUIRE(mat[15] == Catch::Approx(1.0));
//     }

//     SECTION("Get all lines (ids)") {
//         // Preflight to learn required bytes
//         size_t bytes = ifc_api_get_all_lines(api, (uint32_t)modelID, nullptr);
//         REQUIRE(bytes >= sizeof(uint32_t));
//         size_t count = bytes / sizeof(uint32_t);
//         std::vector<uint32_t> ids(count);
//         size_t wrote = ifc_api_get_all_lines(api, (uint32_t)modelID, ids.data());
//         REQUIRE(wrote == bytes);
//         REQUIRE(count > 0);
//     }

//     SECTION("Encode / Decode text roundtrip") {
//         const char *t = "Hello, IFC!";
//         size_t enc_need = ifc_api_encode_text(api, t, nullptr);
//         REQUIRE(enc_need > 0);
//         std::string enc; enc.resize(enc_need);
//         size_t enc_written = ifc_api_encode_text(api, t, enc.data());
//         REQUIRE(enc_written == enc_need);

//         size_t dec_need = ifc_api_decode_text(api, enc.c_str(), nullptr);
//         REQUIRE(dec_need > 0);
//         std::string dec; dec.resize(dec_need);
//         size_t dec_written = ifc_api_decode_text(api, enc.c_str(), dec.data());
//         REQUIRE(dec_written == dec_need);
//         REQUIRE(dec.find("Hello") != std::string::npos);
//     }

//     // Close model and cleanup
//     ifc_api_close_model(api, (uint32_t)modelID);
//     REQUIRE_FALSE(ifc_api_is_model_open(api, (uint32_t)modelID));

//     ifc_api_free(api);
// }
