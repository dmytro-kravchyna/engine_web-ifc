// /*
//  * Translated (representative) functional tests from the TypeScript
//  * suite into C++. These tests exercise a subset of the WebIFC API
//  * (model open/save, encoding, basic geometry streaming and queries)
//  * using the helper implementation in helpers/web_ifc_wasm.cpp.
//  */

// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_test_macros.hpp>
// #include <catch2/catch_approx.hpp>

// #include <nlohmann/json.hpp>
// #include <glm/glm.hpp>

// // Include the helper implementation (pure C++ API used by other tests)
// #include "../helpers/web_ifc_wasm.cpp"

// #include <fstream>
// #include <filesystem>

// using json = nlohmann::json;

// static std::string readFileContents(const std::filesystem::path &p)
// {
//     std::ifstream f(p, std::ios::binary);
//     if (!f)
//         return {};
//     f.seekg(0, std::ios::end);
//     std::streamsize sz = f.tellg();
//     f.seekg(0, std::ios::beg);
//     if (sz <= 0)
//         return {};
//     std::string s; s.resize((size_t)sz);
//     f.read(&s[0], sz);
//     return s;
// }

// static std::string locate_example_ifc()
// {
//     const char* candidates[] = {
//         "../../tests/ifcfiles/public/example.ifc",
//         "../../../tests/ifcfiles/public/example.ifc",
//         "../tests/ifcfiles/public/example.ifc",
//         "tests/ifcfiles/public/example.ifc",
//         "../../tests/ifcfiles/public/AC20-FZK-Haus.ifc"
//     };
//     for (const char* c : candidates) {
//         if (std::filesystem::exists(std::filesystem::path(c)))
//             return std::string(c);
//     }
//     return {};
// }

// TEST_CASE("Encode and decode text roundtrip", "[EncodeDecode]")
// {
//     std::string text = "Hello, world!";
//     std::string enc = EncodeText(text);
//     std::string dec = DecodeText(enc);
//     REQUIRE(dec == text);

//     std::string special = "IFC with newline\n and backslash\\.";
//     enc = EncodeText(special);
//     dec = DecodeText(enc);
//     REQUIRE(dec == special);
// }

// TEST_CASE("Model creation and basic lifecycle", "[Model]")
// {
//     webifc::manager::ModelManager manager(false);
//     webifc::manager::LoaderSettings settings;

//     int model1 = CreateModel(&manager, settings);
//     int model2 = CreateModel(&manager, settings);
//     REQUIRE(model1 != model2);
//     REQUIRE(IsModelOpen(&manager, model1));
//     REQUIRE(IsModelOpen(&manager, model2));

//     CloseModel(&manager, model1);
//     REQUIRE_FALSE(IsModelOpen(&manager, model1));
//     CloseAllModels(&manager);
//     REQUIRE_FALSE(IsModelOpen(&manager, model2));
// }

// TEST_CASE("Open example.ifc and basic queries", "[OpenModel]")
// {
//     auto path = locate_example_ifc();
//     if (path.empty()) {
//         WARN("No example.ifc found in test paths - skipping file-based tests");
//         return;
//     }

//     std::string content = readFileContents(path);
//     REQUIRE_FALSE(content.empty());

//     webifc::manager::ModelManager manager(false);
//     webifc::manager::LoaderSettings settings;

//     int modelID = OpenModel(&manager, settings, [&](char *dest, size_t offset, size_t destSize){
//         size_t remaining = (offset < content.size()) ? (content.size() - offset) : 0;
//         uint32_t length = static_cast<uint32_t>(std::min(remaining, destSize));
//         if (length > 0)
//             memcpy(dest, content.data() + offset, length);
//         return length;
//     });
//     REQUIRE(modelID > 0);
//     REQUIRE(IsModelOpen(&manager, modelID));

//     uint32_t maxId = GetMaxExpressID(&manager, modelID);
//     REQUIRE(maxId > 0);

//     auto coord = GetCoordinationMatrix(&manager, modelID);
//     // Expect diagonal identity by default
//     REQUIRE(coord[0] == Catch::Approx(1.0));
//     REQUIRE(coord[5] == Catch::Approx(1.0));
//     REQUIRE(coord[10] == Catch::Approx(1.0));
//     REQUIRE(coord[15] == Catch::Approx(1.0));

//     // Stream a small number of meshes (if any) to ensure geometry path works
//     size_t meshCount = 0;
//     StreamAllMeshes(&manager, modelID, [&](const webifc::geometry::IfcFlatMesh &mesh, int idx, int total){
//         (void)mesh; (void)idx; (void)total;
//         ++meshCount;
//     });

//     // meshCount may be >0 on real models; we just assert streaming completed
//     SUCCEED();

//     CloseModel(&manager, modelID);
//     REQUIRE_FALSE(IsModelOpen(&manager, modelID));
// }
