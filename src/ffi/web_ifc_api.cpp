// Implementation file for the C API declared in web_ifc_api.h
// Include the C++ ModelManager from the upstream project via an include directory.
// When the real C++ project is present and added as a subdirectory via
// WEBIFC_CPP_DIR, this header resolves through the include path supplied
// by CMake (target_include_directories).  In the absence of the full
// project, a stub header exists at cpp/web-ifc/modelmanager/ModelManager.h
// in the repository root.
#include "web_ifc_api.h"
#include <stdlib.h>
#include <unordered_map>
#include <unordered_set>

// Standard library headers
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <functional>
#include <array>
#include <sstream>

// Third‑party libraries
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

// Web‑IFC headers
#include <web-ifc/modelmanager/ModelManager.h>
#include <version.h>
#include <web-ifc/geometry/operations/bim-geometry/extrusion.h>
#include <web-ifc/geometry/operations/bim-geometry/sweep.h>
#include <web-ifc/geometry/operations/bim-geometry/circularSweep.h>
#include <web-ifc/geometry/operations/bim-geometry/revolution.h>
#include <web-ifc/geometry/operations/bim-geometry/cylindricalRevolution.h>
#include <web-ifc/geometry/operations/bim-geometry/parabola.h>
#include <web-ifc/geometry/operations/bim-geometry/clothoid.h>
#include <web-ifc/geometry/operations/bim-geometry/arc.h>
#include <web-ifc/geometry/operations/bim-geometry/alignment.h>
#include <web-ifc/geometry/operations/bim-geometry/utils.h>
#include <web-ifc/geometry/operations/bim-geometry/boolean.h>
#include <web-ifc/geometry/operations/bim-geometry/profile.h>

#define LOG_HEADER_IMPLEMENTATION
#include "helpers/log.h"
#include "helpers/web_ifc_wasm.cpp"
#include "helpers/strdup.cpp"

/*
 * This source file provides stub implementations for the C API defined
 * in web_ifc_api.h.  Each function mirrors a corresponding method on
 * the TypeScript IfcAPI class in web-ifc-api.ts.  The functions
 * currently perform no real work; they merely return default values
 * such as NULL, zero or false.  Applications embedding this stub may
 * use it as a template for integrating a real WebIFC backend.
 */

static inline webifc::manager::LoaderSettings ifc_api_create_settings(const LoaderSettings *in)
{
  webifc::manager::LoaderSettings s;
  /* Booleans / ints / doubles all use pointer fields; choose caller’s pointer or default’s address */
  s.COORDINATE_TO_ORIGIN = (in && in->COORDINATE_TO_ORIGIN) ? *in->COORDINATE_TO_ORIGIN : DEFAULT_COORDINATE_TO_ORIGIN;
  s.CIRCLE_SEGMENTS = (in && in->CIRCLE_SEGMENTS) ? *in->CIRCLE_SEGMENTS : DEFAULT_CIRCLE_SEGMENTS;
  s.MEMORY_LIMIT = (in && in->MEMORY_LIMIT) ? *in->MEMORY_LIMIT : DEFAULT_MEMORY_LIMIT;
  s.TAPE_SIZE = (in && in->TAPE_SIZE) ? *in->TAPE_SIZE : DEFAULT_TAPE_SIZE;
  s.LINEWRITER_BUFFER = (in && in->LINEWRITER_BUFFER) ? *in->LINEWRITER_BUFFER : DEFAULT_LINEWRITER_BUFFER;
  s.TOLERANCE_PLANE_INTERSECTION = (in && in->TOLERANCE_PLANE_INTERSECTION) ? *in->TOLERANCE_PLANE_INTERSECTION : DEFAULT_TOLERANCE_PLANE_INTERSECTION;
  s.TOLERANCE_PLANE_DEVIATION = (in && in->TOLERANCE_PLANE_DEVIATION) ? *in->TOLERANCE_PLANE_DEVIATION : DEFAULT_TOLERANCE_PLANE_DEVIATION;
  s.TOLERANCE_BACK_DEVIATION_DISTANCE = (in && in->TOLERANCE_BACK_DEVIATION_DISTANCE) ? *in->TOLERANCE_BACK_DEVIATION_DISTANCE : DEFAULT_TOLERANCE_BACK_DEVIATION_DISTANCE;
  s.TOLERANCE_INSIDE_OUTSIDE_PERIMETER = (in && in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER) ? *in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER : DEFAULT_TOLERANCE_INSIDE_OUTSIDE_PERIMETER;
  s.TOLERANCE_SCALAR_EQUALITY = (in && in->TOLERANCE_SCALAR_EQUALITY) ? *in->TOLERANCE_SCALAR_EQUALITY : DEFAULT_TOLERANCE_SCALAR_EQUALITY;
  s.PLANE_REFIT_ITERATIONS = (in && in->PLANE_REFIT_ITERATIONS) ? *in->PLANE_REFIT_ITERATIONS : DEFAULT_PLANE_REFIT_ITERATIONS;
  s.BOOLEAN_UNION_THRESHOLD = (in && in->BOOLEAN_UNION_THRESHOLD) ? *in->BOOLEAN_UNION_THRESHOLD : DEFAULT_BOOLEAN_UNION_THRESHOLD;
  return s;
}

static int find_schema_index(const char *schemaName)
{
  for (size_t i = 0; i < SCHEMA_NAME_ROWS; ++i)
  {
    for (size_t j = 0; j < SCHEMA_NAME_INDEX[i].len; ++j)
    {
      const char *name = SCHEMA_NAME_DATA[SCHEMA_NAME_INDEX[i].off + j];
      if (name && strcmp(name, schemaName) == 0)
        return (int)i;
    }
  }
  return -1;
}

/* C++ implementation of the opaque IfcAPI defined in the header.  This
 * struct is visible only in this translation unit and may freely use
 * STL containers and C++ types.  It mirrors the responsibilities that
 * were previously represented with raw arrays in the header.
 */
struct IfcAPI
{
  std::vector<uint32_t> model_schema_list;
  std::vector<std::string> model_schema_name_list;
  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> deleted_lines;
  // properties left as opaque pointer for now; the high-level TS code
  // stored an object here. Keep as void* to preserve ABI.
  void *properties = nullptr;
  webifc::manager::ModelManager *manager = nullptr;
  // ifcGuidMap: Map<number, Map<string | number, string | number>> = new Map<
  //   number,
  //   Map<string | number, string | number>
  // >();
  std::unordered_map<uint32_t, std::unordered_map<std::string, uint32_t>> guid_to_id;
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::string>> id_to_guid;
};

/* Create a new API object.  Allocates an IfcAPI structure and zeroes
 * its fields. */
extern "C" FFI_EXPORT IfcAPI *ifc_api_new(void)
{
  IfcAPI *api = nullptr;
  try
  {
    api = new IfcAPI();
    api->manager = new webifc::manager::ModelManager(MT_ENABLED);
  }
  catch (...)
  {
    if (api)
      delete api;
    return NULL;
  }
  return api;
}

/* Free an API object and any internal allocations. */
extern "C" FFI_EXPORT void ifc_api_free(IfcAPI *api)
{
  if (!api)
    return;
  /* Destroy C++ manager if present and delete the whole API object. */
  if (api->manager)
  {
    delete api->manager;
    api->manager = nullptr;
  }
  delete api;
}

/* Initializes the WASM module (stub).  Always returns 0. */
extern "C" FFI_EXPORT int ifc_api_init(IfcAPI *api)
{
  if (!api)
    return -1;
  log_set_level(LOG_LEVEL_ERROR);
  return 0;
}

/* Opens multiple models from byte buffers (stub). */
extern "C" FFI_EXPORT uint32_t *ifc_api_open_models(IfcAPI *api,
                                                    const ByteArray *data_sets,
                                                    size_t num_data_sets,
                                                    const LoaderSettings *settings,
                                                    size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Opens a single model from a buffer (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_open_model(IfcAPI *api,
                                                  const ByteArray data,
                                                  const LoaderSettings *settings)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Opens a model by streaming bytes using a callback (stub). */
extern "C" FFI_EXPORT int ifc_api_open_model_from_callback(IfcAPI *api,
                                                           ModelLoadCallback callback,
                                                           void *load_cb_user_data,
                                                           const LoaderSettings *settings)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Retrieves the schema name for a model (stub). */
extern "C" FFI_EXPORT size_t ifc_api_get_model_schema(const IfcAPI *api,
                                                      uint32_t model_id,
                                                      char *out)
{
  if (!api || !api->manager)
    return 0;

  auto result = api->model_schema_name_list.at(model_id);
  return ffi_strdup(result, out);
}

/* Creates a new model (stub). */
extern "C" FFI_EXPORT int ifc_api_create_model(IfcAPI *api,
                                               const NewIfcModel *model,
                                               const LoaderSettings *settings)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Saves a model to a contiguous buffer (stub). */
extern "C" FFI_EXPORT uint8_t *ifc_api_save_model(const IfcAPI *api,
                                                  uint32_t model_id,
                                                  size_t *out_size)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;

  // std::vector<uint8_t> dataBuffer;
  // dataBuffer.reserve(1024);

  // // Append chunks provided by SaveModel into dataBuffer
  // SaveModel(api->manager, model_id, [&](char *src, size_t srcSize)
  //           {
  //   if (srcSize == 0)
  //     return;
  //   size_t old = dataBuffer.size();
  //   dataBuffer.resize(old + srcSize);
  //   std::memcpy(dataBuffer.data() + old, src, srcSize); });

  // // Transfer ownership to a malloc'd buffer for C ABI callers.
  // if (out_size)
  //   *out_size = dataBuffer.size();
  // if (dataBuffer.empty())
  //   return NULL;

  // const std::size_t bytes = ffi_strdup(dataBuffer, (uint8_t *)nullptr);
  // if (bytes == 0)
  //   return NULL;
  // uint8_t *out = (uint8_t *)malloc(bytes);
  // if (!out)
  //   return NULL;
  // ffi_strdup(dataBuffer, out);
  // return out;
}

/* Saves a model by streaming bytes via a callback (stub). */
extern "C" FFI_EXPORT void ifc_api_save_model_to_callback(const IfcAPI *api,
                                                          uint32_t model_id,
                                                          ModelSaveCallback save_cb,
                                                          void *save_cb_user_data)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager || !save_cb)
    return;

  // // Wrap the C-style callback into a std::function that accepts raw buffer
  // SaveModel(api->manager, model_id, [&](char *src, size_t srcSize)
  //           {
  //   if (srcSize == 0)
  //     return;
  //   // Use ffi_strdup raw-bytes overload into a temporary vector to avoid manual malloc/free
  //   std::vector<uint8_t> tmpVec(srcSize);
  //   if (srcSize)
  //     ffi_strdup(src, srcSize, tmpVec.data());
  //   save_cb(tmpVec.data(), srcSize, save_cb_user_data); });
}

/* Retrieves the geometry of an element (stub). */
extern "C" FFI_EXPORT IfcGeometry *ifc_api_get_geometry(const IfcAPI *api,
                                                        uint32_t model_id,
                                                        int geometryExpressID)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
  //     return manager.IsModelOpen(modelID) ? manager.GetGeometryProcessor(modelID)->GetGeometry(expressID) : webifc::geometry::IfcGeometry();
}

/* Creates a new AABB helper (stub). */
extern "C" FFI_EXPORT AABB *ifc_api_create_aabb(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Extrusion helper (stub). */
extern "C" FFI_EXPORT Extrusion *ifc_api_create_extrusion(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Sweep helper (stub). */
extern "C" FFI_EXPORT Sweep *ifc_api_create_sweep(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new CircularSweep helper (stub). */
extern "C" FFI_EXPORT CircularSweep *ifc_api_create_circular_sweep(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Revolution helper (stub). */
extern "C" FFI_EXPORT Revolution *ifc_api_create_revolution(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new CylindricalRevolve helper (stub). */
extern "C" FFI_EXPORT CylindricalRevolve *ifc_api_create_cylindrical_revolution(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Parabola helper (stub). */
extern "C" FFI_EXPORT Parabola *ifc_api_create_parabola(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Clothoid helper (stub). */
extern "C" FFI_EXPORT Clothoid *ifc_api_create_clothoid(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new Arc helper (stub). */
extern "C" FFI_EXPORT Arc *ifc_api_create_arc(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new AlignmentOp helper (stub). */
extern "C" FFI_EXPORT AlignmentOp *ifc_api_create_alignment(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
  //  return bimGeometry::Alignment();
}

/* Creates a new BooleanOperator helper (stub). */
extern "C" FFI_EXPORT BooleanOperator *ifc_api_create_boolean_operator(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
}

/* Creates a new ProfileSection helper (stub). */
extern "C" FFI_EXPORT ProfileSection *ifc_api_create_profile(IfcAPI *api)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
  // return bimGeometry::Profile();
}

/* Gets header line data (stub). */
extern "C" FFI_EXPORT RawLineData ifc_api_get_header_line(const IfcAPI *api,
                                                          uint32_t model_id,
                                                          int headerType)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};
  // if (!manager.IsModelOpen(modelID))
  //         return emscripten::val::undefined();
  //     auto loader = manager.GetIfcLoader(modelID);
  //     auto lines = loader->GetHeaderLinesWithType(headerType);

  //     if (lines.size() <= 0)
  //         return emscripten::val::undefined();
  //     auto line = lines[0];
  //     loader->MoveToHeaderLineArgument(line, 0);

  //     std::string s(manager.GetSchemaManager().IfcTypeCodeToType(headerType));
  //     auto arguments = GetArgs(modelID);
  //     auto retVal = emscripten::val::object();
  //     retVal.set("ID", line);
  //     retVal.set("type", s);
  //     retVal.set("arguments", arguments);
  //     return retVal;
}

/* Gets all types of a model (stub). */
extern "C" FFI_EXPORT IfcType *ifc_api_get_all_types_of_model(const IfcAPI *api,
                                                              uint32_t model_id,
                                                              size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets line data for a single express ID (stub). */
extern "C" FFI_EXPORT void *ifc_api_get_line(const IfcAPI *api,
                                             uint32_t model_id,
                                             uint32_t expressID,
                                             bool flatten,
                                             bool inverse,
                                             const char *inversePropKey)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets line data for multiple express IDs (stub). */
extern "C" FFI_EXPORT void **ifc_api_get_lines(const IfcAPI *api,
                                               uint32_t model_id,
                                               const int *expressIDs,
                                               size_t num_ids,
                                               bool flatten,
                                               bool inverse,
                                               const char *inversePropKey,
                                               size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets the next unused express ID (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_get_next_express_id(const IfcAPI *api,
                                                           uint32_t model_id,
                                                           uint32_t expressID)
{
  if (!api || !api->manager)
    return NULL;
  if (!api->manager->IsModelOpen(model_id))
    return NULL;

  return api->manager->GetIfcLoader(model_id)->GetNextExpressID(expressID);
}

/* Creates a new IFC entity (stub). */
extern "C" FFI_EXPORT void *ifc_api_create_ifc_entity(IfcAPI *api,
                                                      uint32_t model_id,
                                                      int type,
                                                      void *args)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Creates a new IFC globally unique ID (stub). */
extern "C" FFI_EXPORT char *ifc_api_create_ifc_globally_unique_id(IfcAPI *api,
                                                                  uint32_t model_id)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Creates a new IFC type (stub). */
extern "C" FFI_EXPORT void *ifc_api_create_ifc_type(IfcAPI *api,
                                                    uint32_t model_id,
                                                    int type,
                                                    const void *value)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets the name from a type code (stub). */
extern "C" FFI_EXPORT size_t ifc_api_get_name_from_type_code(const IfcAPI *api,
                                                             uint32_t type,
                                                             char *out)
{
  if (!api || !api->manager)
    return NULL;

  std::string result = api->manager->GetSchemaManager().IfcTypeCodeToType(type);
  return ffi_strdup(result, out);
}

/* Gets the type code from a name (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_get_type_code_from_name(const IfcAPI *api,
                                                               const char *type_name)
{
  if (!api || !api->manager)
    return NULL;
  return api->manager->GetSchemaManager().IfcTypeToTypeCode(type_name);
}

/* Evaluates if a type is a subtype of IfcElement (stub). */
extern "C" FFI_EXPORT bool ifc_api_is_ifc_element(const IfcAPI *api,
                                                  int type)
{
  if (!api || !api->manager)
    return false;
  return api->manager->GetSchemaManager().IsIfcElement(type);
}

/* Returns a list of all entity types present in the current schema (stub). */
extern "C" FFI_EXPORT int *ifc_api_get_ifc_entity_list(const IfcAPI *api,
                                                       uint32_t model_id,
                                                       size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Deletes an IFC line (stub). */
extern "C" FFI_EXPORT void ifc_api_delete_line(IfcAPI *api,
                                               uint32_t model_id,
                                               uint32_t expressID)
{
  if (!api || !api->manager)
    return;

  if (api->manager->IsModelOpen(model_id))
    api->manager->GetIfcLoader(model_id)->RemoveLine(expressID);

  api->deleted_lines[model_id].insert(expressID);
}

/* Writes multiple lines (stub). */
extern "C" FFI_EXPORT void ifc_api_write_lines(IfcAPI *api,
                                               uint32_t model_id,
                                               void **line_objects,
                                               size_t num_lines)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return;
}

/* Writes a single line (stub). */
extern "C" FFI_EXPORT void ifc_api_write_line(IfcAPI *api,
                                              uint32_t model_id,
                                              void *line_object)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return;
}

/* Gets all line IDs of a specific type (stub). */
extern "C" FFI_EXPORT int *ifc_api_get_line_ids_with_type(const IfcAPI *api,
                                                          uint32_t model_id,
                                                          int type,
                                                          bool includeInherited,
                                                          size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets all line IDs of a model (stub). */
extern "C" FFI_EXPORT size_t ifc_api_get_all_lines(const IfcAPI *api,
                                                   uint32_t model_id,
                                                   uint32_t *out)
{
  if (!api || !api->manager)
    return 0;
  if (!api->manager->IsModelOpen(model_id))
    return 0;

  std::vector<uint32_t> lineIds = api->manager->GetIfcLoader(model_id)->GetAllLines();
  return ffi_strdup(lineIds, out);
}

/* Gets all 2D cross sections (stub). */
extern "C" FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_2d(const IfcAPI *api,
                                                                      uint32_t model_id,
                                                                      size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets all 3D cross sections (stub). */
extern "C" FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_3d(const IfcAPI *api,
                                                                      uint32_t model_id,
                                                                      size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Gets all alignments (stub). */
extern "C" FFI_EXPORT AlignmentData *ifc_api_get_all_alignments(const IfcAPI *api,
                                                                uint32_t model_id,
                                                                size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Sets the geometry transformation matrix (stub). */
extern "C" FFI_EXPORT void ifc_api_set_geometry_transformation(IfcAPI *api,
                                                               uint32_t model_id,
                                                               const double transformationMatrix[16])
{
  if (!api || !api->manager || !transformationMatrix)
    return;

  std::array<double, 16> m;
  std::memcpy(m.data(), transformationMatrix, sizeof(m));

  if (api->manager->IsModelOpen(model_id))
    api->manager->GetGeometryProcessor(model_id)->SetTransformation(m);
}

/* Gets the coordination matrix (stub). */
extern "C" FFI_EXPORT size_t ifc_api_get_coordination_matrix(const IfcAPI *api,
                                                             uint32_t model_id,
                                                             double *out)
{
  if (!api || !api->manager)
    return 0;
  if (!api->manager->IsModelOpen(model_id))
    return 0;
  std::array<double, 16> arr = api->manager->GetGeometryProcessor(model_id)->GetFlatCoordinationMatrix();
  return ffi_strdup(arr, out);
}

/* Closes a model (stub). */
extern "C" FFI_EXPORT void ifc_api_close_model(IfcAPI *api,
                                               uint32_t model_id)
{
  if (!api || !api->manager)
    return;
  // Erase per-model guid maps if present (unordered_map keyed by model_id)
  api->guid_to_id.erase(model_id);
  api->id_to_guid.erase(model_id);
  api->manager->CloseModel(model_id);
}

/* Disposes all models (stub). */
extern "C" FFI_EXPORT void ifc_api_dispose(IfcAPI *api)
{
  if (!api)
    return;
  api->guid_to_id.clear();
  api->id_to_guid.clear();
  if (api->manager)
    api->manager->CloseAllModels();
}

/* Streams meshes with specific express IDs (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_meshes(const IfcAPI *api,
                                                 uint32_t model_id,
                                                 const int *expressIDs,
                                                 size_t num_ids,
                                                 IfcMeshCallback mesh_cb, //    meshCallback: (mesh: FlatMesh, index: number, total: number) => void

                                                 void *user_data)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return;
}

/* Streams all meshes of a model (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_all_meshes(const IfcAPI *api,
                                                     uint32_t model_id,
                                                     IfcMeshCallback mesh_cb, //    meshCallback: (mesh: FlatMesh, index: number, total: number) => void
                                                     void *user_data)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return;
}

/* Streams all meshes with types (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_all_meshes_with_types(const IfcAPI *api,
                                                                uint32_t model_id,
                                                                const int *types,
                                                                size_t num_types,
                                                                IfcMeshCallback mesh_cb, //    meshCallback: (mesh: FlatMesh, index: number, total: number) => void
                                                                void *user_data)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return;
}

/* Checks if a model is open (stub). */
extern "C" FFI_EXPORT bool ifc_api_is_model_open(const IfcAPI *api,
                                                 uint32_t model_id)
{
  if (!api || !api->manager)
    return false;
  return api->manager->IsModelOpen(model_id);
}

/* Loads all geometry in a model (stub). */
extern "C" FFI_EXPORT FlatMesh **ifc_api_load_all_geometry(const IfcAPI *api,
                                                           uint32_t model_id,
                                                           size_t *out_count)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return NULL;
}

/* Loads geometry for a single element (stub). */
extern "C" FFI_EXPORT FlatMesh *ifc_api_get_flat_mesh(const IfcAPI *api,
                                                      uint32_t model_id,
                                                      uint32_t expressID)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !api->manager)
    return {};

  // if (!api->manager->IsModelOpen(model_id))
  //   return {};
  // webifc::geometry::IfcFlatMesh mesh = api->manager->GetGeometryProcessor(model_id)->GetFlatMesh(expressID);
  // for (auto &geom : mesh.geometries)
  //   api->manager->GetGeometryProcessor(model_id)->GetGeometry(geom.geometryExpressID).GetVertexData();
  // return mesh;
}

/* Gets the maximum express ID (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_get_max_express_id(const IfcAPI *api,
                                                          uint32_t model_id)
{
  if (!api || !api->manager)
    return 0;
  return api->manager->IsModelOpen(model_id) ? api->manager->GetIfcLoader(model_id)->GetMaxExpressId() : 0;
}

/* Gets the line type (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_get_line_type(const IfcAPI *api,
                                                     uint32_t model_id,
                                                     uint32_t expressID)
{
  if (!api || !api->manager)
    return 0;
  return api->manager->IsModelOpen(model_id) ? api->manager->GetIfcLoader(model_id)->GetLineType(expressID) : 0;
}

/* Gets the version of web-ifc (stub). */
extern "C" FFI_EXPORT size_t ifc_api_get_version(const IfcAPI *api, char *out)
{
  return ffi_strdup(WEB_IFC_VERSION_NUMBER, out);
}

// // Build GUID maps for a model. This inspects all entity types, reads the
// // GlobalId where present and stores bidirectional mappings in the API
// // object's per-model maps. This function is intentionally simple and
// // skips entries that cannot be decoded.
// static void build_guid_maps(IfcAPI *api, int modelID)
// {
//   if (!api || !api->manager)
//     return;
//   // Ensure map vectors are large enough
//   if ((int)api->guid_to_id.size() <= modelID)
//     api->guid_to_id.resize(modelID + 1);
//   if ((int)api->id_to_guid.size() <= modelID)
//     api->id_to_guid.resize(modelID + 1);

//   auto &g2i = api->guid_to_id[modelID];
//   auto &i2g = api->id_to_guid[modelID];
//   g2i.clear();
//   i2g.clear();

//   // Use the manager's schema to obtain element types
//   const auto &elementTypes = api->manager->GetSchemaManager().GetIfcElementList();
//   for (uint32_t typeId : elementTypes)
//   {
//     // Get all express IDs for this type
//     auto lines = api->manager->GetIfcLoader(modelID)->GetExpressIDsWithType(typeId);
//     for (uint32_t expressID : lines)
//     {
//       try
//       {
//         json line = GetLine(api->manager, (uint32_t)modelID, expressID);
//         if (!line.is_object())
//           continue;
//         std::string guid;
//         if (line.contains("arguments") && line["arguments"].is_array())
//         {
//           for (const auto &arg : line["arguments"])
//           {
//             if (arg.is_string())
//             {
//               guid = arg.get<std::string>();
//               break;
//             }
//             if (arg.is_object() && arg.contains("value") && arg["value"].is_string())
//             {
//               guid = arg["value"].get<std::string>();
//               break;
//             }
//           }
//         }
//         if (!guid.empty())
//         {
//           g2i[guid] = (int)expressID;
//           i2g[(int)expressID] = guid;
//         }
//       }
//       catch (...)
//       {
//         continue;
//       }
//     }
//   }
// }

/* Looks up express ID from GUID. Returns 0 if not found. */
extern "C" FFI_EXPORT int ifc_api_get_express_id_from_guid(const IfcAPI *api,
                                                           uint32_t model_id,
                                                           const char *guid)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api || !guid)
    return 0;
  // // cast away const to allow lazy population
  // IfcAPI *wapi = const_cast<IfcAPI *>(api);
  // if ((int)wapi->guid_to_id.size() <= model_id || wapi->guid_to_id[model_id].empty())
  // {
  //   build_guid_maps(wapi, model_id);
  // }
  // if ((int)wapi->guid_to_id.size() <= model_id)
  //   return 0;
  // auto &g2i = wapi->guid_to_id[model_id];
  // auto it = g2i.find(std::string(guid));
  // return (it == g2i.end()) ? 0 : it->second;
}

/* Looks up GUID from express ID. Returns NULL if not found. The returned
 * string is owned by the API instance and remains valid until the model is
 * closed or the API is freed. */
extern "C" FFI_EXPORT const char *ifc_api_get_guid_from_express_id(const IfcAPI *api,
                                                                   uint32_t model_id,
                                                                   uint32_t expressID)
{
  // TODO: translate from TypeScript to C++ 20
  if (!api)
    return NULL;

  // IfcAPI *wapi = const_cast<IfcAPI *>(api);
  // if ((int)wapi->id_to_guid.size() <= model_id || wapi->id_to_guid[model_id].empty())
  // {
  //   build_guid_maps(wapi, model_id);
  // }
  // if ((int)wapi->id_to_guid.size() <= model_id)
  //   return NULL;
  // auto &i2g = wapi->id_to_guid[model_id];
  // auto it = i2g.find(expressID);
  // if (it == i2g.end())
  //   return NULL;
  // // store string in a vector inside the API to keep lifetime; reuse map storage
  // return i2g[expressID].c_str();
}

// /* Sets the wasm path (stub). */
// extern "C" FFI_EXPORT void ifc_api_set_wasm_path(IfcAPI *api,
//                           const char *path,
//                           bool absolute) {
//     (void)api; (void)path; (void)absolute;
// }

/* Sets the log level (stub). */
extern "C" FFI_EXPORT void ifc_api_set_log_level(IfcAPI *api,
                                                 LogLevel level)
{
  if (!api)
    return;
  log_set_level(level);
  api->manager->SetLogLevel(static_cast<uint8_t>(level));
}

/* Encodes text using IFC encoding (stub). */
extern "C" FFI_EXPORT size_t ifc_api_encode_text(const IfcAPI *api,
                                                 const char *text,
                                                 char *out)
{
  if (!api || !text)
    return 0;

  const std::string_view strView{text};
  std::ostringstream output;
  webifc::parsing::p21encode(strView, output);
  std::string encoded = output.str();
  return ffi_strdup(encoded, out);
}

/* Decodes text using IFC encoding (stub). */
extern "C" FFI_EXPORT size_t ifc_api_decode_text(const IfcAPI *api,
                                                 const char *text,
                                                 char *out)
{
  if (!api || !text)
    return 0;

  std::string_view sv{text};
  std::string decoded = webifc::parsing::p21decode(const_cast<std::string_view &>(sv));
  return ffi_strdup(decoded, out);
}

/* Resets the cached IFC data (stub). */
extern "C" FFI_EXPORT void ifc_api_reset_cache(const IfcAPI *api,
                                               uint32_t model_id)
{
  if (!api)
    return;

  if (api->manager->IsModelOpen(model_id))
    api->manager->GetGeometryProcessor(model_id)->GetLoader().ResetCache();
}
