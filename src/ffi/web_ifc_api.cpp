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

// Do not include C++ headers here to avoid macro/name collisions with
// the public C headers (e.g. ifc_schema.h). The C++ implementation
// library (web_ifc_cpp) is linked at build time when available and
// provides the actual definitions.


/* Create a new API object.  Memory is allocated with malloc. */
IfcAPI *ifc_api_new(void) {
    IfcAPI *api = (IfcAPI *)calloc(1, sizeof(IfcAPI));
    if (!api) return NULL;
    /* Defer constructing a C++ ModelManager here — some build
     * configurations may provide it via the web_ifc_cpp library.
     * Initialize to NULL to be safe. */
    /* Create a ModelManager when possible so the C API is usable
     * immediately from C++ consumers. If the ModelManager type or
     * library is not available this will still compile because the
     * header is available in the include path for the implementation. */
    api->manager = new webifc::manager::ModelManager(MT_ENABLED);
    return api;
}

/* Free an API object and any internal allocations. */
void ifc_api_free(IfcAPI *api) {
    if (!api) return;
    /* Free model schema names */
    if (api->model_schema_name_list) {
        for (size_t i = 0; i < api->model_schema_count; ++i) {
            free(api->model_schema_name_list[i]);
        }
        free(api->model_schema_name_list);
    }
    /* Free model schema list */
    free(api->model_schema_list);
    /* Free deleted lines arrays */
    if (api->deleted_lines) {
        for (size_t i = 0; i < api->deleted_lines_count; ++i) {
            free(api->deleted_lines[i]);
        }
        free(api->deleted_lines);
    }
    /* Destroy the ModelManager if present. */
    if (api->manager) {
        delete api->manager;
        api->manager = NULL;
    }
    /* Free the API struct */
    free(api);
}

/* Initialize the API.  In this stub implementation this always
 * succeeds and returns 0.  A custom locate file handler may be
 * supplied as a pointer (unused here). */
int ifc_api_init(IfcAPI *api){
    if (!api) return -1;
    log_set_level(LOG_LEVEL_ERROR);
    return 0;
}

/* Open multiple models from byte buffers.  Returns an array of model
 * IDs allocated with malloc and stores its length in out_count.  The
 * returned array and its contents should be freed by the caller. */
int *ifc_api_open_models(IfcAPI *api,
                        const ByteArray* data_sets, /* array of ByteArray */
                        size_t num_data_sets,
                        const LoaderSettings *settings,
                        size_t *out_count) {
    (void)api; (void)data_sets; (void)num_data_sets; (void)settings; (void)out_count;
    return NULL;
}

/* Open a single model from a buffer.  Returns a model ID on success
 * or -1 on failure. */
int ifc_api_open_model(IfcAPI *api,
                        const ByteArray data,
                        const LoaderSettings *settings){
    if (!api || !data.data || data.len == 0) return -1;
    LoaderSettings eff = ifc_api_create_settings(settings);
    (void)eff; (void)data; /* stub: implementation left to library author */
    return -1;
}
