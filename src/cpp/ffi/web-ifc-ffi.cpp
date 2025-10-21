/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "web-ifc-ffi.h"

#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <sstream>

#include "../web-ifc/modelmanager/ModelManager.h"
#include "../web-ifc/geometry/IfcGeometryProcessor.h"
#include "../web-ifc/parsing/IfcLoader.h"
#include "../web-ifc/schema/ifc-schema.h"
#include "../version.h"

using namespace webifc::manager;
using namespace webifc::geometry;
using namespace webifc::parsing;

// Global manager instance similar to wasm binding (no MT for now)
static ModelManager g_manager(false);
static std::mutex g_mutex;

extern "C" {

const char* webifc_get_version() {
    return WEB_IFC_VERSION_NUMBER.data();
}

WebIFC_LoaderSettings webifc_default_loader_settings() {
    WebIFC_LoaderSettings s{};
    s.COORDINATE_TO_ORIGIN = false;
    s.CIRCLE_SEGMENTS = 12;
    s.TAPE_SIZE = 67108864;
    s.MEMORY_LIMIT = 2147483648U;
    s.LINEWRITER_BUFFER = 10000;
    s.TOLERANCE_PLANE_INTERSECTION = 1.0E-04;
    s.TOLERANCE_PLANE_DEVIATION = 1.0E-04;
    s.TOLERANCE_BACK_DEVIATION_DISTANCE = 1.0E-04;
    s.TOLERANCE_INSIDE_OUTSIDE_PERIMETER = 1.0E-10;
    s.TOLERANCE_SCALAR_EQUALITY = 1.0E-04;
    s.PLANE_REFIT_ITERATIONS = 1;
    s.BOOLEAN_UNION_THRESHOLD = 150;
    return s;
}

static LoaderSettings convert(const WebIFC_LoaderSettings &in) {
    LoaderSettings out{};
    out.COORDINATE_TO_ORIGIN = in.COORDINATE_TO_ORIGIN;
    out.CIRCLE_SEGMENTS = in.CIRCLE_SEGMENTS;
    out.TAPE_SIZE = in.TAPE_SIZE;
    out.MEMORY_LIMIT = in.MEMORY_LIMIT;
    out.LINEWRITER_BUFFER = in.LINEWRITER_BUFFER;
    out.TOLERANCE_PLANE_INTERSECTION = in.TOLERANCE_PLANE_INTERSECTION;
    out.TOLERANCE_PLANE_DEVIATION = in.TOLERANCE_PLANE_DEVIATION;
    out.TOLERANCE_BACK_DEVIATION_DISTANCE = in.TOLERANCE_BACK_DEVIATION_DISTANCE;
    out.TOLERANCE_INSIDE_OUTSIDE_PERIMETER = in.TOLERANCE_INSIDE_OUTSIDE_PERIMETER;
    out.TOLERANCE_SCALAR_EQUALITY = in.TOLERANCE_SCALAR_EQUALITY;
    out.PLANE_REFIT_ITERATIONS = in.PLANE_REFIT_ITERATIONS;
    out.BOOLEAN_UNION_THRESHOLD = in.BOOLEAN_UNION_THRESHOLD;
    return out;
}

WebIFC_ModelHandle webifc_create_model(WebIFC_LoaderSettings settings) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_manager.CreateModel(convert(settings));
}

bool webifc_is_model_open(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_manager.IsModelOpen(model);
}

void webifc_close_model(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_manager.CloseModel(model);
}

void webifc_close_all_models() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_manager.CloseAllModels();
}

WebIFC_ErrorCode webifc_load_step_from_memory(WebIFC_ModelHandle model, const char* data, uint32_t length) {
    if (!data || length == 0) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return WEBIFC_ERR_INVALID_MODEL;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return WEBIFC_ERR_INVALID_MODEL;
    try {
        // Use stringstream as adaptor
        std::string s(data, data + length);
        std::istringstream iss(s, std::ios::binary);
        loader->LoadFile(iss);
    } catch (...) {
        return WEBIFC_ERR_INTERNAL;
    }
    return WEBIFC_OK;
}

uint32_t webifc_save_step_to_memory(WebIFC_ModelHandle model, char* outBuffer, uint32_t maxLength, bool orderLinesByExpressID) {
    if (!outBuffer || maxLength == 0) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    try {
        std::ostringstream oss(std::ios::binary);
        loader->SaveFile(oss, orderLinesByExpressID);
        auto outStr = oss.str();
        if (outStr.size() > maxLength) return 0; // buffer too small
        std::memcpy(outBuffer, outStr.data(), outStr.size());
        return static_cast<uint32_t>(outStr.size());
    } catch (...) {
        return 0;
    }
}

uint32_t webifc_get_model_size(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    return static_cast<uint32_t>(loader->GetTotalSize());
}

uint32_t webifc_get_max_express_id(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    return loader->GetMaxExpressId();
}

uint32_t webifc_get_next_express_id(WebIFC_ModelHandle model, uint32_t currentExpressId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    return loader->GetNextExpressID(currentExpressId);
}

bool webifc_validate_express_id(WebIFC_ModelHandle model, uint32_t expressId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return false;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return false;
    return loader->IsValidExpressID(expressId);
}

uint32_t webifc_get_line_type(WebIFC_ModelHandle model, uint32_t expressId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    return loader->GetLineType(expressId);
}

uint32_t webifc_get_line_argument_count(WebIFC_ModelHandle model, uint32_t expressId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return 0;
    return loader->GetNoLineArguments(expressId);
}

static WebIFC_ErrorCode move_to_arg(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex) {
    if (!g_manager.IsModelOpen(model)) return WEBIFC_ERR_INVALID_MODEL;
    auto loader = g_manager.GetIfcLoader(model);
    if (!loader) return WEBIFC_ERR_INVALID_MODEL;
    if (!loader->IsValidExpressID(expressId)) return WEBIFC_ERR_OUT_OF_RANGE;
    try {
        loader->MoveToLineArgument(expressId, argIndex);
    } catch (...) {
        return WEBIFC_ERR_OUT_OF_RANGE;
    }
    return WEBIFC_OK;
}

WebIFC_ErrorCode webifc_get_string_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, char* outBuffer, uint32_t maxLen) {
    if (!outBuffer || maxLen == 0) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto err = move_to_arg(model, expressId, argIndex);
    if (err != WEBIFC_OK) return err;
    auto loader = g_manager.GetIfcLoader(model);
    std::string decoded = loader->GetDecodedStringArgument();
    if (decoded.size() + 1 > maxLen) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::memcpy(outBuffer, decoded.c_str(), decoded.size() + 1);
    return WEBIFC_OK;
}

WebIFC_ErrorCode webifc_get_double_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, double* outValue) {
    if (!outValue) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto err = move_to_arg(model, expressId, argIndex);
    if (err != WEBIFC_OK) return err;
    auto loader = g_manager.GetIfcLoader(model);
    *outValue = loader->GetDoubleArgument();
    return WEBIFC_OK;
}

WebIFC_ErrorCode webifc_get_int_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, int64_t* outValue) {
    if (!outValue) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto err = move_to_arg(model, expressId, argIndex);
    if (err != WEBIFC_OK) return err;
    auto loader = g_manager.GetIfcLoader(model);
    *outValue = loader->GetIntArgument();
    return WEBIFC_OK;
}

WebIFC_ErrorCode webifc_get_ref_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, uint32_t* outValue) {
    if (!outValue) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto err = move_to_arg(model, expressId, argIndex);
    if (err != WEBIFC_OK) return err;
    auto loader = g_manager.GetIfcLoader(model);
    *outValue = loader->GetRefArgument();
    return WEBIFC_OK;
}

WebIFC_ErrorCode webifc_get_flat_mesh(WebIFC_ModelHandle model, uint32_t expressId, WebIFC_FlatMesh* outMesh) {
    if (!outMesh) return WEBIFC_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_manager.IsModelOpen(model)) return WEBIFC_ERR_INVALID_MODEL;
    auto processor = g_manager.GetGeometryProcessor(model);
    if (!processor) return WEBIFC_ERR_INVALID_MODEL;
    try {
        auto flat = processor->GetFlatMesh(expressId, true);
        // Flatten all placed geometries into contiguous arrays (positions only, drop normals for now)
        static std::vector<double> s_vertices; // persistent to keep memory alive
        static std::vector<uint32_t> s_indices; // persistent
        s_vertices.clear();
        s_indices.clear();
        uint32_t vertexOffset = 0;
        for(const auto &placed : flat.geometries) {
            auto &geom = processor->GetGeometry(placed.geometryExpressID);
            // geom.vertexData contains position (3) + normal (3) per vertex
            // copy as-is
            size_t vertsToCopy = geom.vertexData.size();
            s_vertices.insert(s_vertices.end(), geom.vertexData.begin(), geom.vertexData.end());
            // indices adjusted by vertexOffset (in vertex units not doubles)
            for(auto idx : geom.indexData) {
                s_indices.push_back(idx + vertexOffset);
            }
            // vertexOffset increases by number of vertices in this geometry (vertexData stores 6 doubles per vertex)
            vertexOffset += static_cast<uint32_t>(vertsToCopy / webifc::geometry::VERTEX_FORMAT_SIZE_FLOATS);
        }
        outMesh->vertices = s_vertices.data();
        outMesh->vertexCount = static_cast<uint32_t>(s_vertices.size());
        outMesh->indices = s_indices.data();
        outMesh->indexCount = static_cast<uint32_t>(s_indices.size());
    } catch (...) {
        return WEBIFC_ERR_INTERNAL;
    }
    return WEBIFC_OK;
}

void webifc_release_flat_mesh(WebIFC_FlatMesh* mesh) {
    // No ownership transfer; underlying memory managed by geometry processor's internal structures.
    // Provided for future API symmetry; currently nothing to do.
    (void)mesh;
}

// Internal persistent buffers for array returns (reuse to avoid fragmentation). Not thread-safe currently.
static std::vector<uint32_t> s_uint32_buffer;
static std::string s_typename_buffer; // for name lookups if needed

WebIFC_UInt32Array webifc_get_line_ids_with_type(WebIFC_ModelHandle model, uint32_t typeCode) {
    std::lock_guard<std::mutex> lock(g_mutex);
    s_uint32_buffer.clear();
    if(!g_manager.IsModelOpen(model)) return {nullptr,0};
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return {nullptr,0};
    try {
        auto ids = loader->GetExpressIDsWithType(typeCode);
        s_uint32_buffer.insert(s_uint32_buffer.end(), ids.begin(), ids.end());
    } catch(...) {
        return {nullptr,0};
    }
    return { s_uint32_buffer.data(), static_cast<uint32_t>(s_uint32_buffer.size()) };
}

WebIFC_UInt32Array webifc_get_all_line_ids(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    s_uint32_buffer.clear();
    if(!g_manager.IsModelOpen(model)) return {nullptr,0};
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return {nullptr,0};
    try {
        uint32_t maxId = loader->GetMaxExpressId();
        for(uint32_t id=1; id<=maxId; ++id) {
            if(loader->IsValidExpressID(id)) s_uint32_buffer.push_back(id);
        }
    } catch(...) {
        return {nullptr,0};
    }
    return { s_uint32_buffer.data(), static_cast<uint32_t>(s_uint32_buffer.size()) };
}

const char* webifc_get_name_from_type_code(uint32_t typeCode) {
    std::lock_guard<std::mutex> lock(g_mutex);
    s_typename_buffer.clear();
    auto schemaMgr = g_manager.GetSchemaManager(); // returns reference, not pointer
    try {
        s_typename_buffer = schemaMgr.IfcTypeCodeToType(typeCode);
        if(s_typename_buffer.empty()) return nullptr;
        return s_typename_buffer.c_str(); // return pointer valid until next call
    } catch(...) {
        return nullptr;
    }
}

uint32_t webifc_get_type_code_from_name(const char* typeName) {
    if(!typeName) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    try {
        return g_manager.GetSchemaManager().IfcTypeToTypeCode(typeName);
    } catch(...) {
        return 0;
    }
}

bool webifc_is_ifc_element(uint32_t typeCode) {
    std::lock_guard<std::mutex> lock(g_mutex);
    try {
        return g_manager.GetSchemaManager().IsIfcElement(typeCode);
    } catch(...) {
        return false;
    }
}

void webifc_set_log_level(uint8_t level) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_manager.SetLogLevel(level);
}

// Static buffers for text and guid
static std::string s_guid_buffer;
static std::string s_text_buffer;

const char* webifc_generate_guid(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return nullptr;
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return nullptr;
    try {
        s_guid_buffer = loader->GenerateUUID();
        return s_guid_buffer.c_str();
    } catch(...) {
        return nullptr;
    }
}

const char* webifc_encode_text(const char* raw) {
    if(!raw) return nullptr;
    std::lock_guard<std::mutex> lock(g_mutex);
    try {
        // Fallback: no dedicated encoder available in FFI build context; perform minimal escaping of backslash and newline
        std::string_view sv{raw};
        std::string out; out.reserve(sv.size()*2);
        for(char c : sv) {
            if(c=='\\') { out += "\\\\"; }
            else if(c=='\n') { out += "\\n"; }
            else out += c;
        }
        s_text_buffer = out;
        return s_text_buffer.c_str();
    } catch(...) {
        return nullptr;
    }
}

const char* webifc_decode_text(const char* encoded) {
    if(!encoded) return nullptr;
    std::lock_guard<std::mutex> lock(g_mutex);
    try {
        // Minimal decode matching the above escaping
        std::string_view sv{encoded};
        std::string out; out.reserve(sv.size());
        for(size_t i=0;i<sv.size();++i) {
            char c = sv[i];
            if(c=='\\' && i+1<sv.size()) {
                char n = sv[i+1];
                if(n=='n') { out+='\n'; i++; continue; }
                if(n=='\\') { out+='\\'; i++; continue; }
            }
            out+=c;
        }
        s_text_buffer = out;
        return s_text_buffer.c_str();
    } catch(...) {
        return nullptr;
    }
}

bool webifc_set_geometry_transformation(WebIFC_ModelHandle model, const double* matrix16) {
    if(!matrix16) return false;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return false;
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!processor) return false;
    try {
        std::array<double,16> m{};
        for(int i=0;i<16;i++) m[i]=matrix16[i];
        processor->SetTransformation(m);
        return true;
    } catch(...) { return false; }
}

bool webifc_get_coordination_matrix(WebIFC_ModelHandle model, double* outMatrix16) {
    if(!outMatrix16) return false;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return false;
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!processor) return false;
    try {
        auto mat = processor->GetFlatCoordinationMatrix();
        for(int i=0;i<16;i++) outMatrix16[i]=mat[i];
        return true;
    } catch(...) { return false; }
}

void webifc_reset_cache(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return;
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!processor) return;
    try {
        processor->GetLoader().ResetCache();
    } catch(...) { /* swallow */ }
}

WebIFC_UInt32Array webifc_get_line_ids_with_types(WebIFC_ModelHandle model, const uint32_t* typeCodes, uint32_t count) {
    std::lock_guard<std::mutex> lock(g_mutex);
    s_uint32_buffer.clear();
    if(!typeCodes || count==0) return {nullptr,0};
    if(!g_manager.IsModelOpen(model)) return {nullptr,0};
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return {nullptr,0};
    try {
        for(uint32_t i=0;i<count;i++) {
            auto ids = loader->GetExpressIDsWithType(typeCodes[i]);
            s_uint32_buffer.insert(s_uint32_buffer.end(), ids.begin(), ids.end());
        }
    } catch(...) { return {nullptr,0}; }
    return { s_uint32_buffer.data(), static_cast<uint32_t>(s_uint32_buffer.size()) };
}

uint32_t webifc_stream_meshes(WebIFC_ModelHandle model, const uint32_t* expressIds, uint32_t count, WebIFC_StreamMeshCallback cb, void* userData) {
    if(!cb || !expressIds || count==0) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return 0;
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!processor) return 0;
    uint32_t streamed=0;
    try {
        static std::vector<double> s_vertices;
        static std::vector<uint32_t> s_indices;
        WebIFC_FlatMesh tempMesh{};
        for(uint32_t i=0;i<count;i++) {
            auto flat = processor->GetFlatMesh(expressIds[i], true);
            if(flat.geometries.empty()) continue;
            s_vertices.clear(); s_indices.clear();
            uint32_t vertexOffset=0;
            for(const auto &placed : flat.geometries) {
                auto &geom = processor->GetGeometry(placed.geometryExpressID);
                size_t vertsToCopy = geom.vertexData.size();
                s_vertices.insert(s_vertices.end(), geom.vertexData.begin(), geom.vertexData.end());
                for(auto idx : geom.indexData) s_indices.push_back(idx + vertexOffset);
                vertexOffset += static_cast<uint32_t>(vertsToCopy / webifc::geometry::VERTEX_FORMAT_SIZE_FLOATS);
            }
            tempMesh.vertices = s_vertices.data();
            tempMesh.vertexCount = static_cast<uint32_t>(s_vertices.size());
            tempMesh.indices = s_indices.data();
            tempMesh.indexCount = static_cast<uint32_t>(s_indices.size());
            if(tempMesh.vertexCount>0 && tempMesh.indexCount>0) {
                cb(expressIds[i], &tempMesh, userData);
                streamed++;
            }
        }
    } catch(...) { return streamed; }
    return streamed;
}

uint32_t webifc_stream_meshes_with_types(WebIFC_ModelHandle model, const uint32_t* typeCodes, uint32_t count, WebIFC_StreamMeshCallback cb, void* userData) {
    if(!cb || !typeCodes || count==0) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!loader || !processor) return 0;
    uint32_t streamed=0;
    try {
        std::vector<uint32_t> ids;
        for(uint32_t i=0;i<count;i++) {
            auto lineIds = loader->GetExpressIDsWithType(typeCodes[i]);
            ids.insert(ids.end(), lineIds.begin(), lineIds.end());
        }
        streamed = webifc_stream_meshes(model, ids.data(), static_cast<uint32_t>(ids.size()), cb, userData);
    } catch(...) { return streamed; }
    return streamed;
}

uint32_t webifc_stream_all_meshes(WebIFC_ModelHandle model, WebIFC_StreamMeshCallback cb, void* userData, bool skipOpeningsAndSpaces) {
    if(!cb) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!loader || !processor) return 0;
    uint32_t streamed=0;
    try {
        auto &schemaMgr = g_manager.GetSchemaManager();
        auto elementSet = schemaMgr.GetIfcElementList();
        std::vector<uint32_t> types; types.reserve(elementSet.size());
        for(auto t : elementSet) types.push_back(t);
        if(skipOpeningsAndSpaces) {
            types.erase(std::remove_if(types.begin(), types.end(), [](uint32_t t){
                return t==webifc::schema::IFCOPENINGELEMENT || t==webifc::schema::IFCSPACE || t==webifc::schema::IFCOPENINGSTANDARDCASE; }), types.end());
        }
        streamed = webifc_stream_meshes_with_types(model, types.data(), static_cast<uint32_t>(types.size()), cb, userData);
    } catch(...) { return streamed; }
    return streamed;
}

void webifc_remove_line(WebIFC_ModelHandle model, uint32_t expressId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return;
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return;
    try {
        loader->RemoveLine(expressId);
    } catch(...) { /* swallow */ }
}

// ---------------- Phase 2 implementations ----------------
uint32_t webifc_stream_all_flat_meshes(WebIFC_ModelHandle model, WebIFC_StreamMeshCallback cb, void* userData) {
    if(!cb) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if(!g_manager.IsModelOpen(model)) return 0;
    auto processor = g_manager.GetGeometryProcessor(model);
    if(!processor) return 0;
    auto loader = g_manager.GetIfcLoader(model);
    if(!loader) return 0;
    uint32_t streamed = 0;
    try {
        // Iterate all valid express IDs and attempt mesh extraction
        uint32_t maxId = loader->GetMaxExpressId();
        WebIFC_FlatMesh tempMesh{};
        for(uint32_t id=1; id<=maxId; ++id) {
            if(!loader->IsValidExpressID(id)) continue;
            auto flat = processor->GetFlatMesh(id, true);
            if(flat.geometries.empty()) continue;
            // Flatten similar to single mesh code
            static std::vector<double> s_vertices; // reused
            static std::vector<uint32_t> s_indices;
            s_vertices.clear(); s_indices.clear();
            uint32_t vertexOffset = 0;
            // Phase 2 function implementations
            for(const auto &placed : flat.geometries) {
                auto &geom = processor->GetGeometry(placed.geometryExpressID);
                size_t vertsToCopy = geom.vertexData.size();
                s_vertices.insert(s_vertices.end(), geom.vertexData.begin(), geom.vertexData.end());
                for(auto idx : geom.indexData) s_indices.push_back(idx + vertexOffset);
                vertexOffset += static_cast<uint32_t>(vertsToCopy / webifc::geometry::VERTEX_FORMAT_SIZE_FLOATS);
            }
            tempMesh.vertices = s_vertices.data();
            tempMesh.vertexCount = static_cast<uint32_t>(s_vertices.size());
            tempMesh.indices = s_indices.data();
            tempMesh.indexCount = static_cast<uint32_t>(s_indices.size());
            if(tempMesh.vertexCount>0 && tempMesh.indexCount>0) {
                cb(id, &tempMesh, userData);
                streamed++;
            }
        }
    } catch(...) {
        return streamed; // return count until failure
    }
    return streamed;
}

WebIFC_UInt32Array webifc_load_all_geometry(WebIFC_ModelHandle model) {
    std::lock_guard<std::mutex> lock(g_mutex);
    s_uint32_buffer.clear();
    if(!g_manager.IsModelOpen(model)) return {nullptr,0};
    auto processor = g_manager.GetGeometryProcessor(model);
    auto loader = g_manager.GetIfcLoader(model);
    if(!processor || !loader) return {nullptr,0};
    try {
        uint32_t maxId = loader->GetMaxExpressId();
        for(uint32_t id=1; id<=maxId; ++id) {
            if(!loader->IsValidExpressID(id)) continue;
            auto flat = processor->GetFlatMesh(id, true);
            if(!flat.geometries.empty()) s_uint32_buffer.push_back(id);
        }
    } catch(...) {
        return {nullptr,0};
    }
    return { s_uint32_buffer.data(), static_cast<uint32_t>(s_uint32_buffer.size()) };
}

void webifc_release_uint32_array(WebIFC_UInt32Array* arr) {
    if(!arr) return;
    // Our design uses a static buffer; releasing just nulls the view.
    arr->data = nullptr;
    arr->count = 0;
}

} // extern "C"
