/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Opaque handles
typedef uint32_t WebIFC_ModelHandle; // model id within manager

// Loader settings struct (mirror of C++ LoaderSettings)
typedef struct WebIFC_LoaderSettings {
    bool COORDINATE_TO_ORIGIN;
    uint16_t CIRCLE_SEGMENTS;
    uint32_t TAPE_SIZE;
    uint32_t MEMORY_LIMIT;
    uint16_t LINEWRITER_BUFFER;
    double TOLERANCE_PLANE_INTERSECTION;
    double TOLERANCE_PLANE_DEVIATION;
    double TOLERANCE_BACK_DEVIATION_DISTANCE;
    double TOLERANCE_INSIDE_OUTSIDE_PERIMETER;
    double TOLERANCE_SCALAR_EQUALITY;
    uint16_t PLANE_REFIT_ITERATIONS;
    uint16_t BOOLEAN_UNION_THRESHOLD;
} WebIFC_LoaderSettings;

// Geometry simple flat mesh structure (subset for FFI)
typedef struct WebIFC_FlatMesh {
    const double* vertices; // XYZ triplets
    uint32_t vertexCount;   // number of double values / 3 gives number of vertices
    const uint32_t* indices; // triangle indices
    uint32_t indexCount;    // number of indices
} WebIFC_FlatMesh;

// Simple dynamic array return type for uint32 values (express IDs, etc.)
typedef struct WebIFC_UInt32Array {
    const uint32_t* data;
    uint32_t count;
} WebIFC_UInt32Array;

// Callback signature for streaming meshes; user gets expressID and flat mesh reference (transient)
typedef void (*WebIFC_StreamMeshCallback)(uint32_t expressId, const WebIFC_FlatMesh* mesh, void* userData);

// Cross section / alignment structures could be added later; keep minimal for first iteration

// Error codes
typedef enum WebIFC_ErrorCode {
    WEBIFC_OK = 0,
    WEBIFC_ERR_INVALID_MODEL = 1,
    WEBIFC_ERR_INVALID_ARGUMENT = 2,
    WEBIFC_ERR_INTERNAL = 3,
    WEBIFC_ERR_OUT_OF_RANGE = 4
} WebIFC_ErrorCode;

// Version string
const char* webifc_get_version();

// Settings helper
WebIFC_LoaderSettings webifc_default_loader_settings();

// Model lifecycle
WebIFC_ModelHandle webifc_create_model(WebIFC_LoaderSettings settings);
bool webifc_is_model_open(WebIFC_ModelHandle model);
void webifc_close_model(WebIFC_ModelHandle model);
void webifc_close_all_models();

// Load IFC data from memory (STEP text). Returns error code; model must be created first.
WebIFC_ErrorCode webifc_load_step_from_memory(WebIFC_ModelHandle model, const char* data, uint32_t length);

// Save IFC MODEL to memory; caller provides buffer large enough. Returns bytes written or 0 on error.
uint32_t webifc_save_step_to_memory(WebIFC_ModelHandle model, char* outBuffer, uint32_t maxLength, bool orderLinesByExpressID);

// Query model
uint32_t webifc_get_model_size(WebIFC_ModelHandle model); // bytes consumed
uint32_t webifc_get_max_express_id(WebIFC_ModelHandle model);
uint32_t webifc_get_next_express_id(WebIFC_ModelHandle model, uint32_t currentExpressId);
bool webifc_validate_express_id(WebIFC_ModelHandle model, uint32_t expressId);

// Line access
uint32_t webifc_get_line_type(WebIFC_ModelHandle model, uint32_t expressId); // returns type code or 0
uint32_t webifc_get_line_argument_count(WebIFC_ModelHandle model, uint32_t expressId);

// Read line argument (returns type-specific encoding)
// For simplicity we expose numeric arguments directly; strings copied into provided buffer.
WebIFC_ErrorCode webifc_get_string_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, char* outBuffer, uint32_t maxLen);
WebIFC_ErrorCode webifc_get_double_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, double* outValue);
WebIFC_ErrorCode webifc_get_int_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, int64_t* outValue);
WebIFC_ErrorCode webifc_get_ref_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex, uint32_t* outValue);

// Geometry
WebIFC_ErrorCode webifc_get_flat_mesh(WebIFC_ModelHandle model, uint32_t expressId, WebIFC_FlatMesh* outMesh);

// Release any transient geometry allocations if needed (currently no-op, placeholder for future)
void webifc_release_flat_mesh(WebIFC_FlatMesh* mesh);

// Line ID queries
WebIFC_UInt32Array webifc_get_line_ids_with_type(WebIFC_ModelHandle model, uint32_t typeCode);
WebIFC_UInt32Array webifc_get_all_line_ids(WebIFC_ModelHandle model);

// Get human-readable IFC type name from numeric code
const char* webifc_get_name_from_type_code(uint32_t typeCode);

// Geometry bulk operations
// Streams all meshes invoking callback for each flat mesh; returns number of meshes streamed or 0 on error.
uint32_t webifc_stream_all_flat_meshes(WebIFC_ModelHandle model, WebIFC_StreamMeshCallback cb, void* userData);

// Load all geometry; returns array of express IDs that produce geometry (triangle meshes)
WebIFC_UInt32Array webifc_load_all_geometry(WebIFC_ModelHandle model);

// Release an array returned by FFI (deallocate internal buffer)
void webifc_release_uint32_array(WebIFC_UInt32Array* arr);

// Phase 1 parity additions
// Schema/type utilities
uint32_t webifc_get_type_code_from_name(const char* typeName);
bool webifc_is_ifc_element(uint32_t typeCode);

// Logging control
void webifc_set_log_level(uint8_t level);

// GUID generation (returns pointer to static buffer; copy if needed)
const char* webifc_generate_guid(WebIFC_ModelHandle model);

// Text encoding/decoding (STEP p21 escaping)
const char* webifc_encode_text(const char* raw);
const char* webifc_decode_text(const char* encoded);

// Geometry transformation matrix (16 doubles, column-major). Returns true on success.
bool webifc_set_geometry_transformation(WebIFC_ModelHandle model, const double* matrix16);
bool webifc_get_coordination_matrix(WebIFC_ModelHandle model, double* outMatrix16);

// Reset internal geometry cache
void webifc_reset_cache(WebIFC_ModelHandle model);

// Multi-type line ID query; merges results for all provided type codes
WebIFC_UInt32Array webifc_get_line_ids_with_types(WebIFC_ModelHandle model, const uint32_t* typeCodes, uint32_t count);

// Stream specific meshes by express IDs
uint32_t webifc_stream_meshes(WebIFC_ModelHandle model, const uint32_t* expressIds, uint32_t count, WebIFC_StreamMeshCallback cb, void* userData);

// Stream meshes for given IFC types (all elements of those types)
uint32_t webifc_stream_meshes_with_types(WebIFC_ModelHandle model, const uint32_t* typeCodes, uint32_t count, WebIFC_StreamMeshCallback cb, void* userData);

// Stream all meshes with optional filtering (skip openings/spaces similar to WASM). If skipNonSpatial true, skip certain element types.
uint32_t webifc_stream_all_meshes(WebIFC_ModelHandle model, WebIFC_StreamMeshCallback cb, void* userData, bool skipOpeningsAndSpaces);

// Remove a line from the model
void webifc_remove_line(WebIFC_ModelHandle model, uint32_t expressId);

// ----------------------- Phase 2 parity additions -----------------------
// Structured line/header retrieval and inverse property queries, plus write operations.

// Returns number of arguments for a header line type (e.g. FILE_NAME). 0 if not found.
uint32_t webifc_get_header_line_argument_count(WebIFC_ModelHandle model, uint32_t headerType);

// Get a string argument from a header line by index.
WebIFC_ErrorCode webifc_get_header_string_argument(WebIFC_ModelHandle model, uint32_t headerType, uint32_t argIndex, char* outBuffer, uint32_t maxLen);

// Get raw argument token type for a line argument (to allow higher-level reconstruction).
uint32_t webifc_get_line_argument_token_type(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex); // returns enum numeric token or 0xFF on error

// Get token type for header line argument.
uint32_t webifc_get_header_argument_token_type(WebIFC_ModelHandle model, uint32_t headerType, uint32_t argIndex);

// Retrieve set argument (list of uint32 refs) for a given argument treated as SET; returns array (ownership internal until next call).
WebIFC_UInt32Array webifc_get_set_argument(WebIFC_ModelHandle model, uint32_t expressId, uint32_t argIndex);

// Retrieve inverse property references similar to wasm GetInversePropertyForItem.
// targetTypes: array of type codes, position: index in line arguments pointing to property, set: whether property is a SET.
WebIFC_UInt32Array webifc_get_inverse_property(WebIFC_ModelHandle model, uint32_t expressId, const uint32_t* targetTypes, uint32_t targetTypeCount, uint32_t position, bool set);

// Write a line (low-level) similar to wasm WriteLine. arguments is an array of tokenized entries prepared by caller.
// For Phase 2 minimal support we accept a parallel arrays approach: tokenTypes[] and values[] where
// STRING/ENUM uses char* in valuesStrings; REF uses uint32 in valuesRefs; REAL uses double in valuesDoubles; INTEGER uses int64 in valuesInts; SET not yet supported here.
// To keep API simple we start with only primitive arguments; complex/nested objects require multiple write calls.
WebIFC_ErrorCode webifc_write_line(WebIFC_ModelHandle model,
                                   uint32_t expressId,
                                   uint32_t typeCode,
                                   const uint8_t* tokenTypes,
                                   uint32_t argCount,
                                   const char** valuesStrings,
                                   const uint32_t* valuesRefs,
                                   const double* valuesDoubles,
                                   const int64_t* valuesInts);

// Write a header line (FILE_NAME, FILE_DESCRIPTION, FILE_SCHEMA). Arguments only support SET of STRING or primitive for now.
WebIFC_ErrorCode webifc_write_header_line(WebIFC_ModelHandle model,
                                          uint32_t headerType,
                                          const uint8_t* tokenTypes,
                                          uint32_t argCount,
                                          const char** valuesStrings);

// Convenience: get all line IDs with multiple types AND their line types (paired). For reconstruction acceleration.
// Returns express IDs; caller can call get_line_type. Provided as a placeholder for potential future expansion returning struct.
// (Kept here to signal Phase 2 extension point.)
// ------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
