// Implementation file for the C API declared in web_ifc_api.h
// Include the C++ ModelManager from the upstream project via an include directory.
// When the real C++ project is present and added as a subdirectory via
// WEBIFC_CPP_DIR, this header resolves through the include path supplied
// by CMake (target_include_directories).  In the absence of the full
// project, a stub header exists at cpp/web-ifc/modelmanager/ModelManager.h
// in the repository root.
#include <web-ifc/modelmanager/ModelManager.h>
#include "web_ifc_api.h"
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <string>

#include "helpers/web_ifc_wasm.cpp"

/*
 * This source file provides stub implementations for the C API defined
 * in web_ifc_api.h.  Each function mirrors a corresponding method on
 * the TypeScript IfcAPI class in web-ifc-api.ts.  The functions
 * currently perform no real work; they merely return default values
 * such as NULL, zero or false.  Applications embedding this stub may
 * use it as a template for integrating a real WebIFC backend.
 */

static inline webifc::manager::LoaderSettings ifc_api_create_settings(const LoaderSettings* in) {
    webifc::manager::LoaderSettings s;
    /* Booleans / ints / doubles all use pointer fields; choose caller’s pointer or default’s address */
    s.COORDINATE_TO_ORIGIN               = (in && in->COORDINATE_TO_ORIGIN)               ? *in->COORDINATE_TO_ORIGIN               : DEFAULT_COORDINATE_TO_ORIGIN;
    s.CIRCLE_SEGMENTS                    = (in && in->CIRCLE_SEGMENTS)                    ? *in->CIRCLE_SEGMENTS                    : DEFAULT_CIRCLE_SEGMENTS;
    s.MEMORY_LIMIT                       = (in && in->MEMORY_LIMIT)                       ? *in->MEMORY_LIMIT                       : DEFAULT_MEMORY_LIMIT;
    s.TAPE_SIZE                          = (in && in->TAPE_SIZE)                          ? *in->TAPE_SIZE                          : DEFAULT_TAPE_SIZE;
    s.LINEWRITER_BUFFER                  = (in && in->LINEWRITER_BUFFER)                  ? *in->LINEWRITER_BUFFER                  : DEFAULT_LINEWRITER_BUFFER;
    s.TOLERANCE_PLANE_INTERSECTION       = (in && in->TOLERANCE_PLANE_INTERSECTION)       ? *in->TOLERANCE_PLANE_INTERSECTION       : DEFAULT_TOLERANCE_PLANE_INTERSECTION;
    s.TOLERANCE_PLANE_DEVIATION          = (in && in->TOLERANCE_PLANE_DEVIATION)          ? *in->TOLERANCE_PLANE_DEVIATION          : DEFAULT_TOLERANCE_PLANE_DEVIATION;
    s.TOLERANCE_BACK_DEVIATION_DISTANCE  = (in && in->TOLERANCE_BACK_DEVIATION_DISTANCE)  ? *in->TOLERANCE_BACK_DEVIATION_DISTANCE  : DEFAULT_TOLERANCE_BACK_DEVIATION_DISTANCE;
    s.TOLERANCE_INSIDE_OUTSIDE_PERIMETER = (in && in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER) ? *in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER : DEFAULT_TOLERANCE_INSIDE_OUTSIDE_PERIMETER;
    s.TOLERANCE_SCALAR_EQUALITY          = (in && in->TOLERANCE_SCALAR_EQUALITY)          ? *in->TOLERANCE_SCALAR_EQUALITY          : DEFAULT_TOLERANCE_SCALAR_EQUALITY;
    s.PLANE_REFIT_ITERATIONS             = (in && in->PLANE_REFIT_ITERATIONS)             ? *in->PLANE_REFIT_ITERATIONS             : DEFAULT_PLANE_REFIT_ITERATIONS;
    s.BOOLEAN_UNION_THRESHOLD            = (in && in->BOOLEAN_UNION_THRESHOLD)            ? *in->BOOLEAN_UNION_THRESHOLD            : DEFAULT_BOOLEAN_UNION_THRESHOLD;
    return s;
}

static int find_schema_index(const char* schemaName) {
  for (size_t i = 0; i < SCHEMA_NAME_ROWS; ++i) {
    for (size_t j = 0; j < SCHEMA_NAME_INDEX[i].len; ++j) {
      const char* name = SCHEMA_NAME_DATA[SCHEMA_NAME_INDEX[i].off + j];
      if (name && strcmp(name, schemaName) == 0) return (int)i;
    }
  }
  return -1;
}

/* Create a new API object.  Allocates an IfcAPI structure and zeroes
 * its fields. */
extern "C" FFI_EXPORT IfcAPI *ifc_api_new(void) {
//    IfcAPI *api = (IfcAPI *)calloc(1, sizeof(IfcAPI));
//    if (!api) return NULL;
//    api->manager = new webifc::manager::ModelManager(MT_ENABLED);
//    return api;
    
    IfcAPI *api = (IfcAPI*)malloc(sizeof(IfcAPI));
    if (api) {
        memset(api, 0, sizeof(IfcAPI));
    }
    return api;
}

/* Free an API object and any internal allocations. */
extern "C" FFI_EXPORT void ifc_api_free(IfcAPI *api) {
    if (!api) return;
    
//    /* Free model schema names */
//    if (api->model_schema_name_list) {
//        for (size_t i = 0; i < api->model_schema_count; ++i) {
//            free(api->model_schema_name_list[i]);
//        }
//        free(api->model_schema_name_list);
//    }
//    /* Free model schema list */
//    free(api->model_schema_list);
//    /* Free deleted lines arrays */
//    if (api->deleted_lines) {
//        for (size_t i = 0; i < api->deleted_lines_count; ++i) {
//            free(api->deleted_lines[i]);
//        }
//        free(api->deleted_lines);
//    }
//    /* Destroy the ModelManager if present. */
//    if (api->manager) {
//        delete api->manager;
//        api->manager = NULL;
//    }
//    /* Free the API struct */
//    free(api);
    
    free(api);
}

/* Initializes the WASM module (stub).  Always returns 0. */
extern "C" FFI_EXPORT int ifc_api_init(IfcAPI *api) {
    if (!api) return -1;
    log_set_level(LOG_LEVEL_ERROR);
    return 0;
}

/* Opens multiple models from byte buffers (stub). */
extern "C" FFI_EXPORT uint32_t *ifc_api_open_models(IfcAPI *api,
                        const ByteArray* data_sets,
                        size_t num_data_sets,
                        const LoaderSettings *settings,
                        size_t *out_count) {
    (void)api; (void)data_sets; (void)num_data_sets; (void)settings;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Opens a single model from a buffer (stub). */
extern "C" FFI_EXPORT uint32_t ifc_api_open_model(IfcAPI *api,
                        const ByteArray data,
                        const LoaderSettings *settings) {
    (void)api; (void)data; (void)settings;
    return (uint32_t)-1;
}

/* Opens a model by streaming bytes using a callback (stub). */
extern "C" FFI_EXPORT int ifc_api_open_model_from_callback(IfcAPI *api,
                                      ModelLoadCallback callback,
                                      void *load_cb_user_data,
                                      const LoaderSettings *settings) {
    (void)api; (void)callback; (void)load_cb_user_data; (void)settings;
    return -1;
}

/* Retrieves the schema name for a model (stub). */
extern "C" FFI_EXPORT const char *ifc_api_get_model_schema(const IfcAPI *api, int model_id) {
    (void)api; (void)model_id;
    return NULL;
}

/* Creates a new model (stub). */
extern "C" FFI_EXPORT int ifc_api_create_model(IfcAPI *api,
                          const NewIfcModel *model,
                          const LoaderSettings *settings) {
    (void)api; (void)model; (void)settings;
    return -1;
}

/* Saves a model to a contiguous buffer (stub). */
extern "C" FFI_EXPORT uint8_t *ifc_api_save_model(const IfcAPI *api,
                             int model_id,
                             size_t *out_size) {
    (void)api; (void)model_id;
    if (out_size) *out_size = 0;
    return NULL;
}

/* Saves a model by streaming bytes via a callback (stub). */
extern "C" FFI_EXPORT void ifc_api_save_model_to_callback(const IfcAPI *api,
                                     int model_id,
                                     ModelSaveCallback save_cb,
                                     void *save_cb_user_data) {
    (void)api; (void)model_id; (void)save_cb; (void)save_cb_user_data;
    /* no-op */
}

/* Retrieves the geometry of an element (stub). */
extern "C" FFI_EXPORT IfcGeometry *ifc_api_get_geometry(const IfcAPI *api,
                                 int model_id,
                                 int geometryExpressID) {
    (void)api; (void)model_id; (void)geometryExpressID;
    return NULL;
}

/* Creates a new AABB helper (stub). */
extern "C" FFI_EXPORT AABB *ifc_api_create_aabb(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Extrusion helper (stub). */
extern "C" FFI_EXPORT Extrusion *ifc_api_create_extrusion(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Sweep helper (stub). */
extern "C" FFI_EXPORT Sweep *ifc_api_create_sweep(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new CircularSweep helper (stub). */
extern "C" FFI_EXPORT CircularSweep *ifc_api_create_circular_sweep(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Revolution helper (stub). */
extern "C" FFI_EXPORT Revolution *ifc_api_create_revolution(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new CylindricalRevolve helper (stub). */
extern "C" FFI_EXPORT CylindricalRevolve *ifc_api_create_cylindrical_revolution(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Parabola helper (stub). */
extern "C" FFI_EXPORT Parabola *ifc_api_create_parabola(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Clothoid helper (stub). */
extern "C" FFI_EXPORT Clothoid *ifc_api_create_clothoid(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new Arc helper (stub). */
extern "C" FFI_EXPORT Arc *ifc_api_create_arc(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new AlignmentOp helper (stub). */
extern "C" FFI_EXPORT AlignmentOp *ifc_api_create_alignment(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new BooleanOperator helper (stub). */
extern "C" FFI_EXPORT BooleanOperator *ifc_api_create_boolean_operator(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Creates a new ProfileSection helper (stub). */
extern "C" FFI_EXPORT ProfileSection *ifc_api_create_profile(IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Gets header line data (stub). */
extern "C" FFI_EXPORT RawLineData ifc_api_get_header_line(const IfcAPI *api,
                                   int model_id,
                                   int headerType) {
    (void)api; (void)model_id; (void)headerType;
    RawLineData out;
    out.ID = 0;
    out.type = 0;
    out.arguments = NULL;
    out.arguments_len = 0;
    return out;
}

/* Gets all types of a model (stub). */
extern "C" FFI_EXPORT IfcType *ifc_api_get_all_types_of_model(const IfcAPI *api,
                                       int model_id,
                                       size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets line data for a single express ID (stub). */
extern "C" FFI_EXPORT void *ifc_api_get_line(const IfcAPI *api,
                        int model_id,
                        int expressID,
                        bool flatten,
                        bool inverse,
                        const char *inversePropKey) {
    (void)api; (void)model_id; (void)expressID; (void)flatten; (void)inverse; (void)inversePropKey;
    return NULL;
}

/* Gets line data for multiple express IDs (stub). */
extern "C" FFI_EXPORT void **ifc_api_get_lines(const IfcAPI *api,
                          int model_id,
                          const int *expressIDs,
                          size_t num_ids,
                          bool flatten,
                          bool inverse,
                          const char *inversePropKey,
                          size_t *out_count) {
    (void)api; (void)model_id; (void)expressIDs; (void)num_ids;
    (void)flatten; (void)inverse; (void)inversePropKey;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets the next unused express ID (stub). */
extern "C" FFI_EXPORT int ifc_api_get_next_express_id(const IfcAPI *api,
                                 int model_id,
                                 int expressID) {
    (void)api; (void)model_id; (void)expressID;
    return 0;
}

/* Creates a new IFC entity (stub). */
extern "C" FFI_EXPORT void *ifc_api_create_ifc_entity(IfcAPI *api,
                               int model_id,
                               int type,
                               void *args) {
    (void)api; (void)model_id; (void)type; (void)args;
    return NULL;
}

/* Creates a new IFC globally unique ID (stub). */
extern "C" FFI_EXPORT char *ifc_api_create_ifc_globally_unique_id(IfcAPI *api,
                                           int model_id) {
    (void)api; (void)model_id;
    return NULL;
}

/* Creates a new IFC type (stub). */
extern "C" FFI_EXPORT void *ifc_api_create_ifc_type(IfcAPI *api,
                             int model_id,
                             int type,
                             const void *value) {
    (void)api; (void)model_id; (void)type; (void)value;
    return NULL;
}

/* Gets the name from a type code (stub). */
extern "C" FFI_EXPORT const char *ifc_api_get_name_from_type_code(const IfcAPI *api,
                                            int type) {
    (void)api; (void)type;
    return NULL;
}

/* Gets the type code from a name (stub). */
extern "C" FFI_EXPORT int ifc_api_get_type_code_from_name(const IfcAPI *api,
                                   const char *type_name) {
    (void)api; (void)type_name;
    return 0;
}

/* Evaluates if a type is a subtype of IfcElement (stub). */
extern "C" FFI_EXPORT bool ifc_api_is_ifc_element(const IfcAPI *api,
                           int type) {
    (void)api; (void)type;
    return false;
}

/* Returns a list of all entity types present in the current schema (stub). */
extern "C" FFI_EXPORT int *ifc_api_get_ifc_entity_list(const IfcAPI *api,
                                 int model_id,
                                 size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Deletes an IFC line (stub). */
extern "C" FFI_EXPORT void ifc_api_delete_line(IfcAPI *api,
                         int model_id,
                         int expressID) {
    (void)api; (void)model_id; (void)expressID;
}

/* Writes multiple lines (stub). */
extern "C" FFI_EXPORT void ifc_api_write_lines(IfcAPI *api,
                          int model_id,
                          void **line_objects,
                          size_t num_lines) {
    (void)api; (void)model_id; (void)line_objects; (void)num_lines;
}

/* Writes a single line (stub). */
extern "C" FFI_EXPORT void ifc_api_write_line(IfcAPI *api,
                         int model_id,
                         void *line_object) {
    (void)api; (void)model_id; (void)line_object;
}

/* Gets all line IDs of a specific type (stub). */
extern "C" FFI_EXPORT int *ifc_api_get_line_ids_with_type(const IfcAPI *api,
                                     int model_id,
                                     int type,
                                     bool includeInherited,
                                     size_t *out_count) {
    (void)api; (void)model_id; (void)type; (void)includeInherited;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets all line IDs of a model (stub). */
extern "C" FFI_EXPORT int *ifc_api_get_all_lines(const IfcAPI *api,
                          int model_id,
                          size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets all 2D cross sections (stub). */
extern "C" FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_2d(const IfcAPI *api,
                                              int model_id,
                                              size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets all 3D cross sections (stub). */
extern "C" FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_3d(const IfcAPI *api,
                                              int model_id,
                                              size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Gets all alignments (stub). */
extern "C" FFI_EXPORT AlignmentData *ifc_api_get_all_alignments(const IfcAPI *api,
                                        int model_id,
                                        size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Sets the geometry transformation matrix (stub). */
extern "C" FFI_EXPORT void ifc_api_set_geometry_transformation(IfcAPI *api,
                                       int model_id,
                                       const double *transformationMatrix) {
    (void)api; (void)model_id; (void)transformationMatrix;
}

/* Gets the coordination matrix (stub). */
extern "C" FFI_EXPORT double *ifc_api_get_coordination_matrix(const IfcAPI *api,
                                        int model_id,
                                        size_t *out_len) {
    (void)api; (void)model_id;
    if (out_len) *out_len = 0;
    return NULL;
}

/* Closes a model (stub). */
extern "C" FFI_EXPORT void ifc_api_close_model(IfcAPI *api,
                        int model_id) {
    (void)api; (void)model_id;
}

/* Disposes all models (stub). */
extern "C" FFI_EXPORT void ifc_api_dispose(IfcAPI *api) {
    (void)api;
}

/* Streams meshes with specific express IDs (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_meshes(const IfcAPI *api,
                          int model_id,
                          const int *expressIDs,
                          size_t num_ids,
                          IfcMeshCallback mesh_cb,
                          void *user_data) {
    (void)api; (void)model_id; (void)expressIDs; (void)num_ids; (void)mesh_cb; (void)user_data;
}

/* Streams all meshes of a model (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_all_meshes(const IfcAPI *api,
                              int model_id,
                              IfcMeshCallback mesh_cb,
                              void *user_data) {
    (void)api; (void)model_id; (void)mesh_cb; (void)user_data;
}

/* Streams all meshes with types (stub). */
extern "C" FFI_EXPORT void ifc_api_stream_all_meshes_with_types(const IfcAPI *api,
                                         int model_id,
                                         const int *types,
                                         size_t num_types,
                                         IfcMeshCallback mesh_cb,
                                         void *user_data) {
    (void)api; (void)model_id; (void)types; (void)num_types; (void)mesh_cb; (void)user_data;
}

/* Checks if a model is open (stub). */
extern "C" FFI_EXPORT bool ifc_api_is_model_open(const IfcAPI *api,
                          int model_id) {
    (void)api; (void)model_id;
    return false;
}

/* Loads all geometry in a model (stub). */
extern "C" FFI_EXPORT FlatMesh **ifc_api_load_all_geometry(const IfcAPI *api,
                                   int model_id,
                                   size_t *out_count) {
    (void)api; (void)model_id;
    if (out_count) *out_count = 0;
    return NULL;
}

/* Loads geometry for a single element (stub). */
extern "C" FFI_EXPORT FlatMesh *ifc_api_get_flat_mesh(const IfcAPI *api,
                               int model_id,
                               int expressID) {
    (void)api; (void)model_id; (void)expressID;
    return NULL;
}

/* Gets the maximum express ID (stub). */
extern "C" FFI_EXPORT int ifc_api_get_max_express_id(const IfcAPI *api,
                              int model_id) {
    (void)api; (void)model_id;
    return 0;
}

/* Gets the line type (stub). */
extern "C" FFI_EXPORT int ifc_api_get_line_type(const IfcAPI *api,
                          int model_id,
                          int expressID) {
    (void)api; (void)model_id; (void)expressID;
    return 0;
}

/* Gets the version of web-ifc (stub). */
extern "C" FFI_EXPORT const char *ifc_api_get_version(const IfcAPI *api) {
    (void)api;
    return NULL;
}

/* Looks up express ID from GUID (stub). */
extern "C" FFI_EXPORT int ifc_api_get_express_id_from_guid(const IfcAPI *api,
                                     int model_id,
                                     const char *guid) {
    (void)api; (void)model_id; (void)guid;
    return 0;
}

/* Looks up GUID from express ID (stub). */
extern "C" FFI_EXPORT const char *ifc_api_get_guid_from_express_id(const IfcAPI *api,
                                            int model_id,
                                            int expressID) {
    (void)api; (void)model_id; (void)expressID;
    return NULL;
}

/* Sets the wasm path (stub). */
extern "C" FFI_EXPORT void ifc_api_set_wasm_path(IfcAPI *api,
                          const char *path,
                          bool absolute) {
    (void)api; (void)path; (void)absolute;
}

/* Sets the log level (stub). */
extern "C" FFI_EXPORT void ifc_api_set_log_level(IfcAPI *api,
                          int level) {
    (void)api; (void)level;
}

/* Encodes text using IFC encoding (stub). */
extern "C" FFI_EXPORT char *ifc_api_encode_text(const IfcAPI *api,
                          const char *text) {
    (void)api; (void)text;
    return NULL;
}

/* Decodes text using IFC encoding (stub). */
extern "C" FFI_EXPORT char *ifc_api_decode_text(const IfcAPI *api,
                          const char *text) {
    (void)api; (void)text;
    return NULL;
}

/* Resets the cached IFC data (stub). */
extern "C" FFI_EXPORT char *ifc_api_reset_cache(const IfcAPI *api,
                          int model_id) {
    (void)api; (void)model_id;
    return NULL;
}
