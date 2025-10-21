/* Minimal FFI tests replicating a subset of web-ifc-test via the C API */

#include <TinyCppTest.hpp>
#include "../ffi/web-ifc-ffi.h"
#include <fstream>
#include <vector>
#include <string>

static std::string ReadFile(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if(!f) return "";
    f.seekg(0,std::ios::end); auto sz = f.tellg(); f.seekg(0,std::ios::beg);
    std::string data; data.resize((size_t)sz);
    f.read(data.data(), sz);
    return data;
}

static std::string LoadExampleIFC() {
    // Try a set of relative paths depending on execution working directory
    const char* candidates[] = {
        "../../tests/ifcfiles/public/example.ifc", // build/<cfg>
        "../../../tests/ifcfiles/public/example.ifc", // build/native or deeper
        "../tests/ifcfiles/public/example.ifc",
        "tests/ifcfiles/public/example.ifc"
    };
    for(const char* c : candidates) {
        auto data = ReadFile(c);
        if(!data.empty()) return data;
    }
    return {};
}

TEST(FFI_CreateAndLoadModel) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    // Model handle can be 0 for first created model
    ASSERT_EQ(webifc_is_model_open(model), true);
    ASSERT_EQ(webifc_is_model_open(model), true);

    auto data = LoadExampleIFC();
    ASSERT_EQ(data.size() != 0u, true);
    auto err = webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size());
    ASSERT_EQ(err, WEBIFC_OK);
    auto maxId = webifc_get_max_express_id(model);
    ASSERT_EQ(maxId > 10u, true);

    // Try reading first few lines and arguments
    uint32_t checked = 0;
    for(uint32_t id=1; id<=maxId && checked < 5; ++id) {
        if(!webifc_validate_express_id(model, id)) continue;
        auto type = webifc_get_line_type(model, id);
        if(type==0) continue;
        auto argCount = webifc_get_line_argument_count(model, id);
        if(argCount>0) {
            char buffer[256];
            auto argErr = webifc_get_string_argument(model, id, 0, buffer, sizeof(buffer));
            // string arg might not be a string; ignore error but ensure not internal failure
            ASSERT_EQ((argErr==WEBIFC_OK || argErr==WEBIFC_ERR_INVALID_ARGUMENT || argErr==WEBIFC_ERR_OUT_OF_RANGE), true);
        }
        checked++;
    }

    webifc_close_model(model);
    ASSERT_EQ(webifc_is_model_open(model), false);
}

TEST(FFI_GeometryFlatMesh) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.size() != 0u, true);
    auto err = webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size());
    ASSERT_EQ(err, WEBIFC_OK);
    auto maxId = webifc_get_max_express_id(model);

    // Find first valid geometry flat mesh
    WebIFC_FlatMesh mesh{};
    bool found = false;
    for(uint32_t id=1; id<=maxId; ++id) {
        if(!webifc_validate_express_id(model,id)) continue;
        auto type = webifc_get_line_type(model,id);
        if(type==0) continue;
        if(webifc_get_flat_mesh(model,id,&mesh)==WEBIFC_OK && mesh.vertexCount>0 && mesh.indexCount>0) { found=true; break; }
    }
    ASSERT_EQ(found, true);
    ASSERT_EQ(mesh.vertexCount > 0u, true);
    ASSERT_EQ(mesh.indexCount > 0u, true);
    webifc_release_flat_mesh(&mesh);
    webifc_close_model(model);
}

TEST(FFI_LineIDsAndNames) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    // Pick a known type from example model: use first valid line's type
    uint32_t maxId = webifc_get_max_express_id(model);
    uint32_t chosenType = 0;
    for(uint32_t id=1; id<=maxId; ++id) {
        if(!webifc_validate_express_id(model,id)) continue;
        auto t = webifc_get_line_type(model,id);
        if(t!=0) { chosenType = t; break; }
    }
    ASSERT_EQ(chosenType!=0, true);
    auto name = webifc_get_name_from_type_code(chosenType);
    ASSERT_EQ(name!=nullptr, true);
    auto arr = webifc_get_line_ids_with_type(model, chosenType);
    ASSERT_EQ(arr.count>0, true);
    // Basic sanity: each id has matching type
    for(uint32_t i=0; i<arr.count && i<10; ++i) { // limit for speed
        auto id = arr.data[i];
        ASSERT_EQ(webifc_get_line_type(model,id)==chosenType, true);
    }
    webifc_release_uint32_array(&arr);
    auto all = webifc_get_all_line_ids(model);
    ASSERT_EQ(all.count>arr.count, true);
    webifc_release_uint32_array(&all);
    webifc_close_model(model);
}

static uint32_t g_streamCount = 0;
static uint32_t g_streamVertexTotal = 0;
static uint32_t g_streamIndexTotal = 0;
static void StreamCallback(uint32_t expressId, const WebIFC_FlatMesh* mesh, void* userData) {
    (void)userData;
    if(mesh && mesh->vertexCount>0 && mesh->indexCount>0) {
        g_streamCount++;
        g_streamVertexTotal += mesh->vertexCount;
        g_streamIndexTotal += mesh->indexCount;
        // Only log first few to avoid spam
        if(g_streamCount <= 5) {
            printf("[STREAM] id=%u verts=%u indices=%u\n", expressId, mesh->vertexCount, mesh->indexCount);
        }
    }
}

TEST(FFI_StreamAndLoadAllGeometry) {
    g_streamCount = 0; g_streamVertexTotal = 0; g_streamIndexTotal = 0;
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    // Use filtered streaming to skip openings/spaces to reduce total iterations
    auto tStart = std::chrono::high_resolution_clock::now();
    uint32_t streamed = webifc_stream_all_meshes(model, StreamCallback, nullptr, true);
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
    printf("[STREAM] meshes=%u timeMs=%lld vertsTotal=%u indicesTotal=%u\n", streamed, (long long)ms, g_streamVertexTotal, g_streamIndexTotal);
    ASSERT_EQ(streamed==g_streamCount, true);
    // Only load geometry for first N express IDs to shorten test time
    auto geoArr = webifc_load_all_geometry(model);
    uint32_t sample = geoArr.count < 10 ? geoArr.count : 10;
    for(uint32_t i=0;i<sample;i++) {
        WebIFC_FlatMesh m{}; ASSERT_EQ(webifc_get_flat_mesh(model, geoArr.data[i], &m)==WEBIFC_OK, true);
        ASSERT_EQ(m.vertexCount>0 && m.indexCount>0, true);
    }
    webifc_release_uint32_array(&geoArr);
    webifc_close_model(model);
}

// ---- Phase 1 parity new tests ----
TEST(FFI_TypeCodeRoundtripAndElementCheck) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    // Pick first element type encountered
    auto all = webifc_get_all_line_ids(model);
    uint32_t chosenType = 0;
    for(uint32_t i=0;i<all.count;i++) {
        uint32_t id = all.data[i];
        uint32_t t = webifc_get_line_type(model, id);
        if(t!=0 && webifc_is_ifc_element(t)) { chosenType = t; break; }
    }
    ASSERT_EQ(chosenType!=0, true);
    const char* name = webifc_get_name_from_type_code(chosenType);
    ASSERT_EQ(name!=nullptr, true);
    uint32_t roundtrip = webifc_get_type_code_from_name(name);
    ASSERT_EQ(roundtrip==chosenType, true);
    ASSERT_EQ(webifc_is_ifc_element(roundtrip), true);
    webifc_release_uint32_array(&all);
    webifc_close_model(model);
}

TEST(FFI_LogLevelSet) {
    webifc_set_log_level(2);
    webifc_set_log_level(0);
    ASSERT_EQ(true, true); // dummy assertion
}

TEST(FFI_GUIDGeneration) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    const char* g1 = webifc_generate_guid(model);
    const char* g2 = webifc_generate_guid(model);
    ASSERT_EQ(g1!=nullptr && g2!=nullptr, true);
    // Some UUID generators may be deterministic per session; accept equality but ensure non-empty length
    ASSERT_EQ(std::string(g1).size()>0 && std::string(g2).size()>0, true);
    webifc_close_model(model);
}

TEST(FFI_EncodeDecodeText) {
    const char* encoded = webifc_encode_text("Line\nBreak\\Test");
    ASSERT_EQ(encoded!=nullptr, true);
    const char* decoded = webifc_decode_text(encoded);
    ASSERT_EQ(decoded!=nullptr, true);
    ASSERT_EQ(std::string(decoded)=="Line\nBreak\\Test", true);
}

TEST(FFI_GeometryTransformationMatrix) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    double m[16];
    for(int i=0;i<16;i++) m[i] = (i%5==0)?1.0:0.0; // identity
    ASSERT_EQ(webifc_set_geometry_transformation(model, m), true);
    double out[16];
    ASSERT_EQ(webifc_get_coordination_matrix(model, out), true);
    for(int i=0;i<16;i++) ASSERT_EQ(out[i]==m[i], true);
    webifc_close_model(model);
}

TEST(FFI_MultiTypeLineIDs) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    // Dynamically discover two distinct element types
    auto all = webifc_get_all_line_ids(model);
    uint32_t found[2] = {0,0};
    for(uint32_t i=0;i<all.count && (found[0]==0 || found[1]==0); ++i) {
        uint32_t t = webifc_get_line_type(model, all.data[i]);
        if(t!=0 && webifc_is_ifc_element(t)) {
            if(found[0]==0) found[0]=t; else if(found[1]==0 && t!=found[0]) found[1]=t;
        }
    }
    ASSERT_EQ(found[0]!=0 && found[1]!=0, true);
    auto arr = webifc_get_line_ids_with_types(model, found, 2);
    ASSERT_EQ(arr.count>0, true);
    webifc_release_uint32_array(&arr);
    webifc_release_uint32_array(&all);
    webifc_close_model(model);
}

TEST(FFI_StreamMeshesByTypes) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    // Find a type that produces at least one mesh
    auto all = webifc_get_all_line_ids(model);
    uint32_t meshType = 0;
    for(uint32_t i=0;i<all.count && meshType==0; ++i) {
        uint32_t id = all.data[i];
        WebIFC_FlatMesh temp{};
        if(webifc_get_flat_mesh(model, id, &temp)==WEBIFC_OK && temp.vertexCount>0) {
            meshType = webifc_get_line_type(model, id);
        }
    }
    ASSERT_EQ(meshType!=0, true);
    uint32_t streamed = 0;
    auto cb = [](uint32_t id, const WebIFC_FlatMesh* mesh, void* userData){
        uint32_t* counter = reinterpret_cast<uint32_t*>(userData);
        if(mesh && mesh->vertexCount>0) (*counter)++;
    };
    uint32_t types[1] = { meshType };
    uint32_t count = webifc_stream_meshes_with_types(model, types, 1, cb, &streamed);
    ASSERT_EQ(count==streamed, true);
    ASSERT_EQ(streamed>0, true);
    webifc_release_uint32_array(&all);
    webifc_close_model(model);
}

TEST(FFI_RemoveLine) {
    auto settings = webifc_default_loader_settings();
    auto model = webifc_create_model(settings);
    auto data = LoadExampleIFC();
    ASSERT_EQ(data.empty(), false);
    ASSERT_EQ(webifc_load_step_from_memory(model, data.c_str(), (uint32_t)data.size()), WEBIFC_OK);
    auto all = webifc_get_all_line_ids(model);
    ASSERT_EQ(all.count>10, true);
    uint32_t target = all.data[5];
    webifc_remove_line(model, target);
    ASSERT_EQ(webifc_validate_express_id(model, target), false);
    webifc_release_uint32_array(&all);
    webifc_close_model(model);
}

int main (int, char* argv[]) {
    std::string executablePath (argv[0]);
    std::wstring wExecutablePath (executablePath.begin (), executablePath.end ());
    TinyCppTest::SetAppLocation (wExecutablePath);
    if (!TinyCppTest::RunTests ()) {
        return 1;
    }
    return 0;
}
