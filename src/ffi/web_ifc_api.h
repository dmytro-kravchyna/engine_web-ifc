/*
 * web_ifc_api.h
 *
 * A C header that approximates the public API of the TypeScript file
 * `web-ifc-api.ts`.  It provides structure and function declarations
 * but does not implement the full WebIFC logic.  All functions in
 * this interface are intended to be implemented by users who wish to
 * parse and write IFC data from C.  The goal is to mirror the shape
 * of the JavaScript API while staying portable across common
 * platforms such as iOS, Android, Windows, Linux and macOS.
 */

#ifndef WEB_IFC_API_H
#define WEB_IFC_API_H

#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint8_t, uint32_t, uint64_t */
#include <stdbool.h> /* bool */
#include <time.h>    /* clock_gettime/timespec_get */
#if !defined(_WIN32)
#include <sys/time.h>
#else
#include <windows.h>
#endif

#include "ifc_schema.h"
#include "helpers/log.h"

/* Platform-specific export */
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
/* When building the DLL define WEB_IFC_API_BUILD in your CMake for the target */
#ifdef WEB_IFC_API_BUILD
#define FFI_EXPORT __declspec(dllexport)
#else
#define FFI_EXPORT __declspec(dllimport)
#endif
#elif defined(__EMSCRIPTEN__)
/* Keep the symbol alive through link-time GC */
#include <emscripten/emscripten.h>
#define FFI_EXPORT EMSCRIPTEN_KEEPALIVE
#elif defined(__GNUC__) || defined(__clang__)
/* GCC/Clang (Linux, Android NDK, Apple clang) */
#define FFI_EXPORT __attribute__((visibility("default")))
#else
#define FFI_EXPORT
#endif

/* Enable multithreading on native platforms. On Emscripten only enable
 * if pthreads are explicitly enabled. The ModelManager constructor takes
 * a boolean _mt_enabled parameter. */
#if defined(__EMSCRIPTEN__)
#if defined(__EMSCRIPTEN_PTHREADS__)
#define MT_ENABLED 1
#else
#define MT_ENABLED 0
#endif
#else
#define MT_ENABLED 1
#endif

#ifdef __cplusplus
/* Forward declare ModelManager to avoid forcing inclusion of C++ headers
 * from this C header. The implementation file and C++ consumers may
 * include the full C++ headers as needed. */
namespace webifc
{
    namespace manager
    {
        class ModelManager;
    }
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    static const char UNKNOWN = 0;
    static const char STRING = 1;
    static const char LABEL = 2;
    static const char ENUM = 3;
    static const char REAL = 4;
    static const char REF = 5;
    static const char EMPTY = 6;
    static const char SET_BEGIN = 7;
    static const char SET_END = 8;
    static const char LINE_END = 9;
    static const char INTEGER = 10;

    /**
     * Settings for the IFCLoader.
     *
     * Each field is optional (may be NULL) and when NULL the loader will use
     * its internal default for that option.
     *
     * Properties:
     *  - COORDINATE_TO_ORIGIN: If true, the model will be translated to the origin.
     *  - CIRCLE_SEGMENTS: Number of segments used to approximate circles.
     *  - MEMORY_LIMIT: Maximum memory (in bytes) to be reserved for IFC data in memory.
     *  - TAPE_SIZE: Size of the internal buffer tape for the loader (in bytes or loader-specific units).
     *  - LINEWRITER_BUFFER: Number of lines to write to memory at a time when writing an IFC file.
     *  - TOLERANCE_PLANE_INTERSECTION: Numerical tolerance when checking plane intersections.
     *  - TOLERANCE_PLANE_DEVIATION: Tolerance to consider a plane on a boundary.
     *  - TOLERANCE_BACK_DEVIATION_DISTANCE: Used to determine if a point lies in front or behind a plane based on normal orientation.
     *  - TOLERANCE_INSIDE_OUTSIDE_PERIMETER: Tolerance for point-in-perimeter calculations.
     *  - TOLERANCE_SCALAR_EQUALITY: Tolerance used to compare scalar values as equal.
     *  - PLANE_REFIT_ITERATIONS: Number of iterations used when adjusting triangles to a plane.
     *  - BOOLEAN_UNION_THRESHOLD: Minimum number of solids before triggering a boolean union operation.
     */
    typedef struct
    {
        const bool *COORDINATE_TO_ORIGIN;                 /* If true, translate model to the origin. */
        const uint16_t *CIRCLE_SEGMENTS;                  /* Segments used to approximate circles. */
        const uint32_t *MEMORY_LIMIT;                     /* Max memory for IFC data in bytes. */
        const uint32_t *TAPE_SIZE;                        /* Internal buffer tape size (bytes/units). */
        const uint16_t *LINEWRITER_BUFFER;                /* Lines to buffer when writing IFC files. */
        const double *TOLERANCE_PLANE_INTERSECTION;       /* Tolerance for plane intersection checks. */
        const double *TOLERANCE_PLANE_DEVIATION;          /* Tolerance to consider a plane on a boundary. */
        const double *TOLERANCE_BACK_DEVIATION_DISTANCE;  /* Threshold to decide front/back of a plane. */
        const double *TOLERANCE_INSIDE_OUTSIDE_PERIMETER; /* Tolerance for point-in-perimeter tests. */
        const double *TOLERANCE_SCALAR_EQUALITY;          /* Tolerance for scalar equality comparisons. */
        const uint16_t *PLANE_REFIT_ITERATIONS;           /* Iterations for refitting triangles to a plane. */
        const uint16_t *BOOLEAN_UNION_THRESHOLD;          /* Min solids before performing boolean union. */
    } LoaderSettings;

    /* Define default values with uppercase names */
    static const bool DEFAULT_COORDINATE_TO_ORIGIN = false;
    static const uint16_t DEFAULT_CIRCLE_SEGMENTS = 12U;
    static const uint32_t DEFAULT_MEMORY_LIMIT = 2147483648U; /* 2 GiB */
    static const uint32_t DEFAULT_TAPE_SIZE = 67108864U;      /* 64 MiB */
    static const uint16_t DEFAULT_LINEWRITER_BUFFER = 10000U;
    static const double DEFAULT_TOLERANCE_PLANE_INTERSECTION = 1.0e-4;
    static const double DEFAULT_TOLERANCE_PLANE_DEVIATION = 1.0e-4;
    static const double DEFAULT_TOLERANCE_BACK_DEVIATION_DISTANCE = 1.0e-4;
    static const double DEFAULT_TOLERANCE_INSIDE_OUTSIDE_PERIMETER = 1.0e-10;
    static const double DEFAULT_TOLERANCE_SCALAR_EQUALITY = 1.0e-4;
    static const uint16_t DEFAULT_PLANE_REFIT_ITERATIONS = 1U;
    static const uint16_t DEFAULT_BOOLEAN_UNION_THRESHOLD = 150U;

    typedef struct
    {
        double *data;
        size_t len;
    } DoubleArray;
    typedef struct
    {
        uint32_t *data;
        size_t len;
    } UInt32Array;
    typedef struct
    {
        int *data;
        size_t len;
    } IntArray;
    typedef struct
    {
        char **data;
        size_t len;
    } StringArray;

    typedef struct
    {
        double x, y, z, w;
    } Color;

    typedef struct
    {
        int ID;
        int type;
        void **arguments; /* any[]; your app decides element types */
        size_t arguments_len;
    } RawLineData;

    typedef struct
    {
        Color color;
        int geometryExpressID;
        double flatTransformation[16]; /* row-major 4x4 matrix */
    } PlacedGeometry;

    typedef struct
    {
        PlacedGeometry *data;
        size_t size;
    } PlacedGeometryVector;

    typedef struct FlatMesh
    {
        PlacedGeometryVector geometries;
        uint32_t expressID;
        void (*destroy)(struct FlatMesh *self); /* optional */
    } FlatMesh;

    /* Point with optional z (NULL => 2D) */
    typedef struct
    {
        double x, y;
        const double *z; /* optional */
    } Point;

    typedef struct
    {
        Point *data;
        size_t len;
    } PointArray;
    typedef struct
    {
        double *data;
        size_t len;
    } NumberArray;
    typedef struct
    {
        char **data;
        size_t len;
    } UserDataArray;

    typedef struct Curve
    {
        PointArray points;       /* Array<Point> */
        UserDataArray userData;  /* Array<string> */
        NumberArray arcSegments; /* Array<number> */
    } Curve;

    typedef struct
    {
        Curve *data;
        size_t len;
    } CurveArray;

    /* Forward for recursive Profile */
    struct Profile;
    typedef struct
    {
        struct Profile *data;
        size_t len;
    } ProfileArray;

    typedef struct Profile
    {
        Curve curve;
        CurveArray holes;      /* Array<Curve> */
        ProfileArray profiles; /* Array<Profile> */
        bool isConvex;
        bool isComposite;
    } Profile;

    typedef struct
    {
        CurveArray curves;  /* Array<Curve> */
        IntArray expressID; /* Array<number> */
    } CrossSection;

    typedef struct
    {
        CurveArray curves;
    } AlignmentSegment;

    typedef struct
    {
        DoubleArray FlatCoordinationMatrix; /* Array<number> */
        AlignmentSegment Horizontal;
        AlignmentSegment Vertical;
        AlignmentSegment Absolute;
    } AlignmentData;

    typedef struct
    {
        Profile profile;
        CurveArray axis; /* Array<Curve> */
        double profileRadius;
    } SweptDiskSolid;

    /* ========== Geometry buffers ========== */
    typedef struct
    {
        DoubleArray fvertexData; /* vertex floats (stored as double here) */
        UInt32Array indexData;   /* indices */
    } Buffers;

    /* ========== Interfaces with methods (function-pointer style) ========== */
    typedef struct IfcGeometry
    {
        size_t (*GetVertexData)(struct IfcGeometry *self);
        size_t (*GetVertexDataSize)(struct IfcGeometry *self);
        size_t (*GetIndexData)(struct IfcGeometry *self);
        size_t (*GetIndexDataSize)(struct IfcGeometry *self);
        SweptDiskSolid (*GetSweptDiskSolid)(struct IfcGeometry *self);
        void (*destroy)(struct IfcGeometry *self);
        void *user;
    } IfcGeometry;

    typedef struct AABB
    {
        Buffers (*GetBuffers)(struct AABB *self);
        void (*SetValues)(struct AABB *self,
                          double minX, double minY, double minZ,
                          double maxX, double maxY, double maxZ);
        void *user;
    } AABB;

    typedef struct Extrusion
    {
        Buffers (*GetBuffers)(struct Extrusion *self);
        void (*SetValues)(struct Extrusion *self,
                          const double *profile, size_t profile_len,
                          const double *dir, size_t dir_len,
                          double len,
                          const double *cuttingPlaneNormal, size_t cuttingPlaneNormal_len,
                          const double *cuttingPlanePos, size_t cuttingPlanePos_len,
                          bool cap);
        void (*SetHoles)(struct Extrusion *self, const double *profile, size_t profile_len);
        void (*ClearHoles)(struct Extrusion *self);
        void *user;
    } Extrusion;

    typedef struct Sweep
    {
        Buffers (*GetBuffers)(struct Sweep *self);
        void (*SetValues)(struct Sweep *self,
                          double scaling,
                          const bool *closed, /* optional */
                          const double *profile, size_t profile_len,
                          const double *directrix, size_t directrix_len,
                          const double *initialNormal, size_t initialNormal_len, /* optional */
                          const bool *rotate90,                                  /* optional */
                          const bool *optimize);                                 /* optional */
        void *user;
    } Sweep;

    typedef struct CircularSweep
    {
        Buffers (*GetBuffers)(struct CircularSweep *self);
        void (*SetValues)(struct CircularSweep *self,
                          double scaling,
                          const bool *closed, /* optional */
                          const double *profile, size_t profile_len,
                          double radius,
                          const double *directrix, size_t directrix_len,
                          const double *initialNormal, size_t initialNormal_len, /* optional */
                          const bool *rotate90);                                 /* optional */
        void *user;
    } CircularSweep;

    typedef struct Revolution
    {
        Buffers (*GetBuffers)(struct Revolution *self);
        void (*SetValues)(struct Revolution *self,
                          const double *profile, size_t profile_len,
                          const double *transform, size_t transform_len,
                          double startDegrees,
                          double endDegrees,
                          int numRots);
        void *user;
    } Revolution;

    typedef struct CylindricalRevolve
    {
        Buffers (*GetBuffers)(struct CylindricalRevolve *self);
        void (*SetValues)(struct CylindricalRevolve *self,
                          const double *transform, size_t transform_len,
                          double startDegrees, double endDegrees,
                          double minZ, double maxZ,
                          int numRots,
                          double radius);
        void *user;
    } CylindricalRevolve;

    typedef struct Parabola
    {
        Buffers (*GetBuffers)(struct Parabola *self);
        void (*SetValues)(struct Parabola *self,
                          int segments,
                          double startPointX, double startPointY, double startPointZ,
                          double horizontalLength,
                          double startHeight,
                          double startGradient,
                          double endGradient);
        void *user;
    } Parabola;

    typedef struct Clothoid
    {
        Buffers (*GetBuffers)(struct Clothoid *self);
        void (*SetValues)(struct Clothoid *self,
                          int segments,
                          double startPointX, double startPointY, double startPointZ,
                          double ifcStartDirection,
                          double StartRadiusOfCurvature,
                          double EndRadiusOfCurvature,
                          double SegmentLength);
        void *user;
    } Clothoid;

    typedef struct Arc
    {
        Buffers (*GetBuffers)(struct Arc *self);
        void (*SetValues)(struct Arc *self,
                          double radiusX, double radiusY,
                          int numSegments,
                          const double *placement, size_t placement_len,
                          const double *startRad,            /* optional */
                          const double *endRad,              /* optional */
                          const bool *swap,                  /* optional */
                          const bool *normalToCenterEnding); /* optional */
        void *user;
    } Arc;

    /* Second TS “Alignment” with methods; renamed to avoid collision with data holder */
    typedef struct AlignmentOp
    {
        Buffers (*GetBuffers)(struct AlignmentOp *self);
        void (*SetValues)(struct AlignmentOp *self,
                          const double *horizontal, size_t horizontal_len,
                          const double *vertical, size_t vertical_len);
        void *user;
    } AlignmentOp;

    typedef struct BooleanOperator
    {
        Buffers (*GetBuffers)(struct BooleanOperator *self);
        void (*SetValues)(struct BooleanOperator *self,
                          const double *triangles, size_t triangles_len,
                          const char *type_); /* "union" / "diff" / "intersect" */
        void (*SetSecond)(struct BooleanOperator *self,
                          const double *triangles, size_t triangles_len);
        void (*clear)(struct BooleanOperator *self);
        void *user;
    } BooleanOperator;

    typedef struct ProfileSection
    {
        Buffers (*GetBuffers)(struct ProfileSection *self);
        void (*SetValues)(struct ProfileSection *self,
                          int pType,
                          double width,
                          double depth,
                          double webThickness,
                          double flangeThickness,
                          const bool *hasFillet, /* optional */
                          double filletRadius,
                          double radius,
                          double slope,
                          int circleSegments,
                          const double *placement, size_t placement_len);
        void *user;
    } ProfileSection;

    /* ========== Meta and model ========== */
    typedef struct
    {
        int typeID;
        const char *typeName; /* not owned */
    } IfcType;

    typedef struct
    {
        const char *schema;        /* required */
        const char *name;          /* optional (NULL) */
        StringArray description;   /* optional (len==0 or NULL) */
        StringArray authors;       /* optional */
        StringArray organizations; /* optional */
        const char *authorization; /* optional (NULL) */
    } NewIfcModel;

    /* ========== I/O callbacks ========== */
    typedef struct
    {
        uint8_t *data;
        size_t len;
    } ByteArray;
    typedef ByteArray (*ModelLoadCallback)(size_t offset, size_t size, void *user);
    typedef void (*ModelSaveCallback)(const uint8_t *data, size_t len, void *user);
    /* (path, prefix) -> absolute path/URL; return UTF-8 C string (policy up to app) */
    // typedef const char* (*LocateFileHandlerFn)(const char* path, const char* prefix, void* user);

    /* ========== ms(): milliseconds since Unix epoch (like TS ms) ========== */
    static inline uint64_t ms(void)
    {
#if defined(_WIN32)
        FILETIME ft;
        ULARGE_INTEGER u;
        const uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL; /* 1601->1970 */
        GetSystemTimeAsFileTime(&ft);
        u.LowPart = ft.dwLowDateTime;
        u.HighPart = ft.dwHighDateTime;
        uint64_t t100 = (u.QuadPart - EPOCH_DIFF_100NS);
        return t100 / 10000ULL; /* 100 ns -> ms */
#elif defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000u);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000u + (uint64_t)(tv.tv_usec / 1000u);
#endif
    }

    // /* Represents a second‑level map entry in the ifcGuidMap.  Keys and
    //  * values can be either numeric or string.  The boolean flags
    //  * indicate which member of the union is valid. */
    // typedef struct {
    //     bool key_is_string;
    //     bool value_is_string;
    //     union {
    //         char *strKey;
    //         int   intKey;
    //     } key;
    //     union {
    //         char *strValue;
    //         int   intValue;
    //     } value;
    // } GuidMapItem;

    // /* A container for all key/value pairs associated with a given outer
    //  * key in the ifcGuidMap.  The outer key is always an integer as
    //  * defined in the TypeScript signature. */
    // typedef struct {
    //     int key;
    //     GuidMapItem *items;
    //     size_t len;
    // } GuidMapEntry;

    // /* A dynamic array of GuidMapEntry structures representing the top
    //  * level of the nested map in ifcGuidMap.  Each entry holds a unique
    //  * integer key and an array of GuidMapItem pairs. */
    // typedef struct {
    //     GuidMapEntry *entries;
    //     size_t len;
    // } IfcGuidMap;

    // /* A container for deleted lines.  The outer key (first) is an
    //  * integer and each associated value is a set of integers. */
    // typedef struct {
    //     int key;
    //     IntArray set;
    // } DeletedLinesEntry;

    // /* A dynamic array of DeletedLinesEntry structures. */
    // typedef struct {
    //     DeletedLinesEntry *entries;
    //     size_t len;
    //     size_t capacity;
    // } DeletedLinesMap;

    // /* Representation of the TypeScript IfcAPI class.  This struct holds
    //  * dynamic arrays for schema lists and name lists, along with nested
    //  * map types for GUID mapping and deleted lines tracking. */
    // typedef struct {
    //     /* List of numeric schema identifiers. */
    //     IntArray modelSchemaList;
    //     /* List of schema names corresponding to each ID. */
    //     StringArray modelSchemaNameList;
    //     /* Map from a numeric key to a nested map of string|number to
    //      * string|number.  See GuidMapItem for representation details. */
    //     IfcGuidMap ifcGuidMap;
    //     /* Map from a numeric key to a set of numeric values representing
    //      * deleted lines. */
    //     DeletedLinesMap deletedLines;
    // } IfcAPI;

    /* Main API structure storing library state.
     *
     * The concrete definition is intentionally omitted from this C header so
     * the implementation can use modern C++ containers without exposing them
     * to C callers.  The type is declared as an incomplete struct; users of
     * the C API only ever manipulate pointers to `IfcAPI` via the exported
     * functions (e.g. ifc_api_new/ifc_api_free).
     */
    typedef struct IfcAPI IfcAPI;

    /* Create a new API object.  Memory is allocated with malloc. */
    FFI_EXPORT IfcAPI *ifc_api_new(void);

    /* Free an API object and any internal allocations. */
    FFI_EXPORT void ifc_api_free(IfcAPI *api);

    /**
     * Initializes the WASM module (WebIFCWasm), required before using any other
     * functionality.  In this stub implementation the function always
     * succeeds and returns 0.  A custom locate file handler may be supplied
     * as a pointer (unused here).
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns 0 on success, non‑zero on failure.
     */
    FFI_EXPORT int ifc_api_init(IfcAPI *api);

    /**
     * Opens a set of models and returns their model IDs.
     *
     * @param api          API context pointer created with ifc_api_new.
     * @param data_sets    Array of ByteArray structures containing IFC data (bytes).
     * @param num_data_sets Number of items in the data_sets array.
     * @param settings     Optional loader settings; may be NULL to use defaults.
     * @param out_count    Output parameter receiving the number of model IDs returned.
     * @returns A malloc'd array of model IDs.  The caller is responsible for freeing
     *          the returned array.  If no models could be opened the function
     *          returns NULL and *out_count is set to zero.
     */
    FFI_EXPORT uint32_t *ifc_api_open_models(IfcAPI *api,
                                             const ByteArray *data_sets,
                                             size_t num_data_sets,
                                             const LoaderSettings *settings,
                                             size_t *out_count);

    /**
     * Opens a model from a single memory buffer and returns a model ID number.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param data     Buffer containing IFC data (bytes).
     * @param settings Optional loader settings; may be NULL to use defaults.
     * @returns ModelID on success or (uint32_t)-1 if the model fails to open.
     */
    FFI_EXPORT uint32_t ifc_api_open_model(IfcAPI *api,
                                           const ByteArray data,
                                           const LoaderSettings *settings);

    /**
     * Opens a model by streaming bytes using a user provided callback and
     * returns a model ID number.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param callback A function of signature `(offset, size) -> ByteArray` that
     *                 retrieves IFC data chunks.  The callback is invoked
     *                 repeatedly until the model is fully read.  The user may
     *                 pass arbitrary context via load_cb_user_data.
     * @param load_cb_user_data User supplied pointer passed back to callback.
     * @param settings Optional loader settings; may be NULL to use defaults.
     * @returns ModelID on success or -1 if the model fails to open.
     */
    FFI_EXPORT int ifc_api_open_model_from_callback(IfcAPI *api,
                                                    ModelLoadCallback callback,
                                                    void *load_cb_user_data,
                                                    const LoaderSettings *settings);

    /**
     * Fetches the IFC schema version of a given model.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model ID obtained from open or create calls.
     * @returns The IFC schema name string.  The returned string is owned by
     *          the API and must not be freed by the caller.  NULL is
     *          returned if the schema is unknown.
     */
    FFI_EXPORT const char *ifc_api_get_model_schema(const IfcAPI *api, uint32_t model_id);

    /**
     * Creates a new model and returns a modelID number.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model    Pointer to a NewIfcModel structure describing the new model.
     * @param settings Optional loader settings; may be NULL to use defaults.
     * @returns ModelID on success or -1 on error.
     */
    FFI_EXPORT int ifc_api_create_model(IfcAPI *api,
                                        const NewIfcModel *model,
                                        const LoaderSettings *settings);

    /**
     * Saves a model to an in‑memory buffer.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model ID to save.
     * @param out_size Pointer that receives the size of the returned buffer in bytes.
     * @returns A malloc'd buffer containing the serialized IFC model.  The
     *          caller is responsible for freeing the returned buffer.  If
     *          the model could not be saved the function returns NULL and
     *          *out_size is set to zero.
     */
    FFI_EXPORT uint8_t *ifc_api_save_model(const IfcAPI *api,
                                           uint32_t model_id,
                                           size_t *out_size);

    /**
     * Saves a model by streaming bytes via a callback.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model ID to save.
     * @param save_cb  A function that receives blocks of serialized IFC bytes.
     *                 The callback is invoked repeatedly until the entire
     *                 model has been output.  User context may be passed
     *                 through save_cb_user_data.
     * @param save_cb_user_data User supplied pointer passed back to the callback.
     */
    FFI_EXPORT void ifc_api_save_model_to_callback(const IfcAPI *api,
                                                   uint32_t model_id,
                                                   ModelSaveCallback save_cb,
                                                   void *save_cb_user_data);

    /* === Additional API functions mirroring web-ifc-api.ts public methods === */

    /**
     * Retrieves the geometry of an element.
     *
     * @param api               API context pointer created with ifc_api_new.
     * @param model_id          Model handle retrieved by OpenModel.
     * @param geometryExpressID Express ID of the element whose geometry is requested.
     * @returns A pointer to an IfcGeometry object containing vertex and index data
     *          for the element, or NULL on failure.  The caller should invoke
     *          the destroy function on the returned object when done.
     */
    FFI_EXPORT IfcGeometry *ifc_api_get_geometry(const IfcAPI *api,
                                                 uint32_t model_id,
                                                 int geometryExpressID);

    /**
     * Creates a new axis‑aligned bounding box (AABB) helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new AABB object or NULL if creation failed.  The
     *          returned object exposes GetBuffers and SetValues methods.
     */
    FFI_EXPORT AABB *ifc_api_create_aabb(IfcAPI *api);

    /**
     * Creates a new extrusion helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Extrusion object or NULL if creation failed.
     */
    FFI_EXPORT Extrusion *ifc_api_create_extrusion(IfcAPI *api);

    /**
     * Creates a new sweep helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Sweep object or NULL if creation failed.
     */
    FFI_EXPORT Sweep *ifc_api_create_sweep(IfcAPI *api);

    /**
     * Creates a new circular sweep helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new CircularSweep object or NULL if creation failed.
     */
    FFI_EXPORT CircularSweep *ifc_api_create_circular_sweep(IfcAPI *api);

    /**
     * Creates a new revolution helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Revolution object or NULL if creation failed.
     */
    FFI_EXPORT Revolution *ifc_api_create_revolution(IfcAPI *api);

    /**
     * Creates a new cylindrical revolution helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new CylindricalRevolve object or NULL if creation failed.
     */
    FFI_EXPORT CylindricalRevolve *ifc_api_create_cylindrical_revolution(IfcAPI *api);

    /**
     * Creates a new parabola helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Parabola object or NULL if creation failed.
     */
    FFI_EXPORT Parabola *ifc_api_create_parabola(IfcAPI *api);

    /**
     * Creates a new clothoid helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Clothoid object or NULL if creation failed.
     */
    FFI_EXPORT Clothoid *ifc_api_create_clothoid(IfcAPI *api);

    /**
     * Creates a new arc helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new Arc object or NULL if creation failed.
     */
    FFI_EXPORT Arc *ifc_api_create_arc(IfcAPI *api);

    /**
     * Creates a new alignment helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new AlignmentOp object or NULL if creation failed.
     */
    FFI_EXPORT AlignmentOp *ifc_api_create_alignment(IfcAPI *api);

    /**
     * Creates a new boolean operator helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new BooleanOperator object or NULL if creation failed.
     */
    FFI_EXPORT BooleanOperator *ifc_api_create_boolean_operator(IfcAPI *api);

    /**
     * Creates a new profile section helper object.
     *
     * @param api API context pointer created with ifc_api_new.
     * @returns A pointer to a new ProfileSection object or NULL if creation failed.
     */
    FFI_EXPORT ProfileSection *ifc_api_create_profile(IfcAPI *api);

    /**
     * Gets the header information required by the user.
     *
     * @param api        API context pointer created with ifc_api_new.
     * @param model_id   Model handle retrieved by OpenModel.
     * @param headerType Type of header data you want to retrieve (e.g. FILE_NAME,
     *                   FILE_DESCRIPTION or FILE_SCHEMA).
     * @returns A RawLineData structure with parameters ID, type and arguments.  On
     *          failure the returned structure will have ID set to zero and
     *          arguments_len equal to zero.
     */
    FFI_EXPORT RawLineData ifc_api_get_header_line(const IfcAPI *api,
                                                   uint32_t model_id,
                                                   int headerType);

    /**
     * Gets the list of all IFC types contained in the model.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter that will receive the number of types returned.
     * @returns An array of IfcType objects allocated with malloc.  The caller is
     *          responsible for freeing the returned array and any nested
     *          allocations.  If no types are available the function returns
     *          NULL and sets *out_count to zero.
     */
    FFI_EXPORT IfcType *ifc_api_get_all_types_of_model(const IfcAPI *api,
                                                       uint32_t model_id,
                                                       size_t *out_count);

    /**
     * Gets the IFC line data for a given express ID.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Express ID of the line.
     * @param flatten   If true, recursively flatten the line (default false).
     * @param inverse   If true, retrieve the inverse properties of the line (default false).
     * @param inversePropKey Optional key used to filter inverse properties for performance; may be NULL.
     * @returns A pointer to an opaque line object representing the IFC line.  The
     *          caller must cast or interpret the returned pointer according to
     *          their own data structures.  NULL is returned on error.
     */
    FFI_EXPORT void *ifc_api_get_line(const IfcAPI *api,
                                      uint32_t model_id,
                                      uint32_t expressID,
                                      bool flatten,
                                      bool inverse,
                                      const char *inversePropKey);

    /**
     * Gets the IFC line data for a list of express IDs.
     *
     * @param api         API context pointer created with ifc_api_new.
     * @param model_id    Model handle retrieved by OpenModel.
     * @param expressIDs  Array of express IDs.
     * @param num_ids     Number of elements in the expressIDs array.
     * @param flatten     If true, recursively flatten the lines (default false).
     * @param inverse     If true, retrieve the inverse properties of the lines (default false).
     * @param inversePropKey Optional key used to filter inverse properties for performance; may be NULL.
     * @param out_count   Output parameter that will receive the number of line objects returned.
     * @returns An array of opaque pointers representing IFC line objects.  The
     *          caller is responsible for freeing the returned array.  If no
     *          lines are returned the function returns NULL and sets *out_count
     *          to zero.
     */
    FFI_EXPORT void **ifc_api_get_lines(const IfcAPI *api,
                                        uint32_t model_id,
                                        const int *expressIDs,
                                        size_t num_ids,
                                        bool flatten,
                                        bool inverse,
                                        const char *inversePropKey,
                                        size_t *out_count);

    /**
     * Gets the next unused express ID starting from the specified value.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Starting express ID value.
     * @returns The next unused express ID starting from the value provided.
     */
    FFI_EXPORT int ifc_api_get_next_express_id(const IfcAPI *api,
                                               uint32_t model_id,
                                               uint32_t expressID);

    /**
     * Creates a new IFC entity of the specified type.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param type     IFC type code.
     * @param args     Opaque pointer to an array of arguments required by the entity.
     *                 The meaning and number of arguments depend on the IFC type and
     *                 are left to the caller.  This pointer may be NULL if no
     *                 arguments are provided.
     * @returns A pointer to an opaque IFC line object representing the new entity,
     *          or NULL on error.
     */
    FFI_EXPORT void *ifc_api_create_ifc_entity(IfcAPI *api,
                                               uint32_t model_id,
                                               int type,
                                               void *args);

    /**
     * Creates a new IFC globally unique ID.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @returns A newly generated globally unique ID string, or NULL on error.
     *          The returned string is malloc'd and must be freed by the caller.
     */
    FFI_EXPORT char *ifc_api_create_ifc_globally_unique_id(IfcAPI *api,
                                                           uint32_t model_id);

    /**
     * Creates a new IFC type, such as IfcLabel or IfcReal.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param type     IFC type code.
     * @param value    Pointer to the value to assign to the type.  The caller is
     *                 responsible for ensuring that the value has the correct
     *                 representation for the specified type.
     * @returns A pointer to an opaque IFC type object, or NULL on error.
     */
    FFI_EXPORT void *ifc_api_create_ifc_type(IfcAPI *api,
                                             uint32_t model_id,
                                             int type,
                                             const void *value);

    /**
     * Gets the name corresponding to a type code.
     *
     * Writes the type name as a NULL‑terminated UTF‑8 string into the
     * caller-supplied buffer `out`. If `out` is NULL the function returns the
     * number of bytes required to hold the string (excluding the trailing NUL).
     * On success the function returns the number of bytes written (excluding
     * the trailing NUL).
     *
     * @param api  API context pointer created with ifc_api_new.
     * @param type IFC type code.
     * @param out  Caller-supplied buffer to receive the name, or NULL for preflight.
     * @returns Number of bytes written or required; zero on error or if the type is unknown.
     */
    FFI_EXPORT size_t ifc_api_get_name_from_type_code(const IfcAPI *api,
                                                      uint32_t type,
                                                      char *out);

    /**
     * Gets the type code from a type name.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param type_name UTF‑8 string containing the type name.
     * @returns The type code corresponding to the name, or 0 if not found.
     */
    FFI_EXPORT uint32_t ifc_api_get_type_code_from_name(const IfcAPI *api,
                                                   const char *type_name);

    /**
     * Evaluates whether a type is a subtype of IfcElement.
     *
     * @param api  API context pointer created with ifc_api_new.
     * @param type IFC type code.
     * @returns true if the type is a subtype of IfcElement; false otherwise.
     */
    FFI_EXPORT bool ifc_api_is_ifc_element(const IfcAPI *api,
                                           int type);

    /**
     * Returns a list with all entity types that are present in the current schema.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of type codes returned.
     * @returns An array of type codes (integers).  The caller is responsible for
     *          freeing the returned array.  If no types are present the
     *          function returns NULL and sets *out_count to zero.
     */
    FFI_EXPORT int *ifc_api_get_ifc_entity_list(const IfcAPI *api,
                                                uint32_t model_id,
                                                size_t *out_count);

    /**
     * Deletes an IFC line from the model.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Express ID of the line to remove.
     */
    FFI_EXPORT void ifc_api_delete_line(IfcAPI *api,
                                        uint32_t model_id,
                                        uint32_t expressID);

    /**
     * Writes a set of lines to the model, which can be used to write new lines
     * or to update existing lines.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param line_objects Array of pointers to opaque line objects to write.
     * @param num_lines Number of objects in the line_objects array.
     */
    FFI_EXPORT void ifc_api_write_lines(IfcAPI *api,
                                        uint32_t model_id,
                                        void **line_objects,
                                        size_t num_lines);

    /**
     * Writes a single line to the model.  This can be used to write a new line
     * or update an existing line.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param line_object Pointer to an opaque line object to write.
     */
    FFI_EXPORT void ifc_api_write_line(IfcAPI *api,
                                       uint32_t model_id,
                                       void *line_object);

    /**
     * Get all line IDs of a specific IFC type.
     *
     * @param api              API context pointer created with ifc_api_new.
     * @param model_id         Model handle retrieved by OpenModel.
     * @param type             IFC type code.
     * @param includeInherited If true, also returns all inherited types (default false).
     * @param out_count        Output parameter receiving the number of line IDs returned.
     * @returns An array of line IDs (integers).  The caller is responsible for
     *          freeing the returned array.  If no IDs are found the function
     *          returns NULL and sets *out_count to zero.
     */
    FFI_EXPORT int *ifc_api_get_line_ids_with_type(const IfcAPI *api,
                                                   uint32_t model_id,
                                                   int type,
                                                   bool includeInherited,
                                                   size_t *out_count);

    /**
     * Get all line IDs of a model.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of line IDs returned.
     * @returns An array of all line IDs (integers).  The caller is responsible
     *          for freeing the returned array.  If no IDs are found the function
     *          returns NULL and sets *out_count to zero.
     */
    /**
     * Gets all line IDs of a model.
     *
     * Writes the list of line IDs into the caller-supplied buffer `out` as
     * 32-bit signed integers. If `out` is NULL the function returns the
     * number of bytes required to hold the list (preflight). On success the
     * function returns the number of bytes written. The out_count parameter
     * receives the number of elements (not bytes) when non-NULL.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out      Caller-supplied buffer to receive the int array, or NULL for preflight.
     * @returns Number of bytes written or required; zero on error.
     */
    FFI_EXPORT size_t ifc_api_get_all_lines(const IfcAPI *api,
                                           uint32_t model_id,
                                           uint32_t *out);

    /**
     * Returns all cross sections in 2D contained in IFCSECTIONEDSOLID,
     * IFCSECTIONEDSURFACE or IFCSECTIONEDSOLIDHORIZONTAL (IFC4x3 or superior).
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of cross sections returned.
     * @returns An array of CrossSection structures.  The caller is responsible
     *          for freeing the returned array and any nested allocations.  If no
     *          cross sections are available the function returns NULL and sets
     *          *out_count to zero.
     */
    FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_2d(const IfcAPI *api,
                                                               uint32_t model_id,
                                                               size_t *out_count);

    /**
     * Returns all cross sections in 3D contained in IFCSECTIONEDSOLID,
     * IFCSECTIONEDSURFACE or IFCSECTIONEDSOLIDHORIZONTAL (IFC4x3 or superior).
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of cross sections returned.
     * @returns An array of CrossSection structures representing 3D curves.
     *          The caller is responsible for freeing the returned array and any
     *          nested allocations.  If no cross sections are available the function
     *          returns NULL and sets *out_count to zero.
     */
    FFI_EXPORT CrossSection *ifc_api_get_all_cross_sections_3d(const IfcAPI *api,
                                                               uint32_t model_id,
                                                               size_t *out_count);

    /**
     * Returns all alignments contained in the IFC model (IFC4x3 or superior).
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of alignments returned.
     * @returns An array of AlignmentData structures.  The caller is responsible
     *          for freeing the returned array and any nested allocations.  If no
     *          alignments are available the function returns NULL and sets
     *          *out_count to zero.
     */
    FFI_EXPORT AlignmentData *ifc_api_get_all_alignments(const IfcAPI *api,
                                                         uint32_t model_id,
                                                         size_t *out_count);

    /**
     * Sets the transformation matrix for geometry.
     *
     * This function requires a fixed-size flat 4x4 matrix supplied as an array
     * of 16 doubles. The function signature documents the fixed-size array so
     * callers on the C side may pass a `double m[16]` buffer directly. The
     * function will have no effect if a NULL pointer is supplied.
     *
     * @param api                 API context pointer created with ifc_api_new.
     * @param model_id            Model handle retrieved by OpenModel.
     * @param transformationMatrix Flat 4x4 matrix as an array of 16 doubles.
     */
    FFI_EXPORT void ifc_api_set_geometry_transformation(IfcAPI *api,
                                                        uint32_t model_id,
                                                        const double transformationMatrix[16]);

    /**
     * Gets the coordination matrix for a model.
     *
     * This variant writes the flat 4x4 coordination matrix into the caller-
     * supplied output buffer. If `out` is NULL the function returns the
     * number of bytes required to hold the matrix (preflight). On success the
     * function returns the number of bytes written (normally 16 * sizeof(double)).
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param out      Pointer to a buffer of at least 16 doubles, or NULL for preflight.
     * @returns Number of bytes written or required; zero on error.
     */
    FFI_EXPORT size_t ifc_api_get_coordination_matrix(const IfcAPI *api,
                                                      uint32_t model_id,
                                                      double *out);

    /**
     * Closes a model and frees all related memory.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.  The model must be
     *                 closed after use.
     */
    FFI_EXPORT void ifc_api_close_model(IfcAPI *api,
                                        uint32_t model_id);

    /**
     * Closes all models and frees all related memory.  Please note that after
     * calling this you must call ifc_api_init() again to ensure web-ifc is in
     * a working state.
     *
     * @param api API context pointer created with ifc_api_new.
     */
    FFI_EXPORT void ifc_api_dispose(IfcAPI *api);

    /**
     * Streams meshes of a model with specific express IDs.
     *
     * @param api         API context pointer created with ifc_api_new.
     * @param model_id    Model handle retrieved by OpenModel.
     * @param expressIDs  Array of express IDs whose meshes should be streamed.
     * @param num_ids     Number of elements in the expressIDs array.
     * @param mesh_cb     Callback function invoked for each mesh.  The callback
     *                    receives a pointer to a FlatMesh, the index of the mesh
     *                    within the stream, the total number of meshes, and the
     *                    user_data pointer supplied here.
     * @param user_data   User supplied pointer passed back to the callback.
     */
    typedef void (*IfcMeshCallback)(const FlatMesh *mesh, size_t index, size_t total, void *user_data);
    FFI_EXPORT void ifc_api_stream_meshes(const IfcAPI *api,
                                          uint32_t model_id,
                                          const int *expressIDs,
                                          size_t num_ids,
                                          IfcMeshCallback mesh_cb,
                                          void *user_data);

    /**
     * Streams all meshes of a model.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param mesh_cb   Callback function invoked for each mesh.  The callback
     *                  receives a pointer to a FlatMesh, the index of the mesh
     *                  within the stream, the total number of meshes, and the
     *                  user_data pointer supplied here.
     * @param user_data User supplied pointer passed back to the callback.
     */
    FFI_EXPORT void ifc_api_stream_all_meshes(const IfcAPI *api,
                                              uint32_t model_id,
                                              IfcMeshCallback mesh_cb,
                                              void *user_data);

    /**
     * Streams all meshes of a model with a specific set of IFC types.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param types     Array of IFC type codes to stream.
     * @param num_types Number of elements in the types array.
     * @param mesh_cb   Callback function invoked for each mesh.  The callback
     *                  receives a pointer to a FlatMesh, the index of the mesh
     *                  within the stream, the total number of meshes, and the
     *                  user_data pointer supplied here.
     * @param user_data User supplied pointer passed back to the callback.
     */
    FFI_EXPORT void ifc_api_stream_all_meshes_with_types(const IfcAPI *api,
                                                         uint32_t model_id,
                                                         const int *types,
                                                         size_t num_types,
                                                         IfcMeshCallback mesh_cb,
                                                         void *user_data);

    /**
     * Checks if a specific model ID is open.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @returns true if the model is open; false if it is closed.
     */
    FFI_EXPORT bool ifc_api_is_model_open(const IfcAPI *api,
                                          uint32_t model_id);

    /**
     * Loads all geometry in a model.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param out_count Output parameter receiving the number of FlatMesh objects returned.
     * @returns An array of pointers to FlatMesh objects.  The caller is responsible
     *          for freeing the returned array.  Each FlatMesh must be destroyed
     *          via its destroy member when no longer needed.  If no geometry is
     *          loaded the function returns NULL and sets *out_count to zero.
     */
    FFI_EXPORT FlatMesh **ifc_api_load_all_geometry(const IfcAPI *api,
                                                    uint32_t model_id,
                                                    size_t *out_count);

    /**
     * Loads geometry for a single element.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Express ID of the element.
     * @returns A pointer to a FlatMesh object or NULL on error.  The caller
     *          should destroy the returned object via its destroy member when
     *          finished.
     */
    FFI_EXPORT FlatMesh *ifc_api_get_flat_mesh(const IfcAPI *api,
                                               uint32_t model_id,
                                               uint32_t expressID);

    /**
     * Returns the maximum ExpressID value in the IFC file (e.g. #9999999).
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @returns The maximum express ID value.
     */
    FFI_EXPORT uint32_t ifc_api_get_max_express_id(const IfcAPI *api,
                                                   uint32_t model_id);

    /**
     * Returns the IFC type of a given entity in the file.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Express ID of the line.
     * @returns IFC type code for the line, or 0 if unknown.
     */
    FFI_EXPORT uint32_t ifc_api_get_line_type(const IfcAPI *api,
                                              uint32_t model_id,
                                              uint32_t expressID);

    /**
     * Returns the version number of web-ifc.
     *
     * Writes a NULL‑terminated UTF‑8 string into the caller-supplied buffer.
     * If `out` is NULL the function returns the number of bytes required
     * (excluding the terminating NUL). On success the function returns the
     * number of bytes written (excluding the terminating NUL).
     *
     * @param api API context pointer created with ifc_api_new.
     * @param out Caller-supplied buffer to receive the version string, or NULL.
     * @returns Number of bytes written or required; zero on error.
     */
    FFI_EXPORT size_t ifc_api_get_version(const IfcAPI *api, char *out);

    /**
     * Looks up an entity's express ID from its GlobalID.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @param guid     UTF‑8 string containing the GlobalID to look up.
     * @returns The express ID corresponding to the given GlobalID, or 0 if not
     *          found.
     */
    FFI_EXPORT int ifc_api_get_express_id_from_guid(const IfcAPI *api,
                                                    uint32_t model_id,
                                                    const char *guid);

    /**
     * Looks up an entity's GlobalID from its ExpressID.
     *
     * @param api       API context pointer created with ifc_api_new.
     * @param model_id  Model handle retrieved by OpenModel.
     * @param expressID Express ID to look up.
     * @returns The GlobalID string associated with the express ID, or NULL if not
     *          found.  The returned string is owned by the API and must not be
     *          freed by the caller.
     */
    FFI_EXPORT const char *ifc_api_get_guid_from_express_id(const IfcAPI *api,
                                                            uint32_t model_id,
                                                            uint32_t expressID);

    /**
     * Sets the path to the wasm file used by the underlying WebIFCWasm module.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param path     UTF‑8 string containing the new wasm path.  The string is
     *                 copied internally and may be freed by the caller after the
     *                 call returns.
     * @param absolute If true, the path is treated as absolute; otherwise it is
     *                 interpreted relative to the executing script.
     */
    // FFI_EXPORT void ifc_api_set_wasm_path(IfcAPI *api,
    //                           const char *path,
    //                           bool absolute);

    /**
     * Sets the log level for diagnostic output.
     *
     * @param api   API context pointer created with ifc_api_new.
     * @param level Log level to set.  See the LogLevel enumeration for valid values.
     */
    FFI_EXPORT void ifc_api_set_log_level(IfcAPI *api,
                                          LogLevel level);

    /**
     * Encodes text using IFC encoding.
     *
     * Writes the encoded text as a NULL‑terminated UTF‑8 string into the
     * caller-supplied buffer. If `out` is NULL the function returns the number
     * of bytes required (excluding the terminating NUL). On success the
     * function returns the number of bytes written (excluding the terminating NUL).
     *
     * @param api  API context pointer created with ifc_api_new.
     * @param text NULL‑terminated UTF‑8 string to encode.
     * @param out  Caller-supplied buffer to receive the encoded text, or NULL.
     * @returns Number of bytes written or required; zero on error.
     */
    FFI_EXPORT size_t ifc_api_encode_text(const IfcAPI *api,
                                          const char *text,
                                          char *out);

    /**
     * Decodes text using IFC encoding.
     *
     * Writes the decoded text as a NULL‑terminated UTF‑8 string into the
     * caller-supplied buffer. If `out` is NULL the function returns the number
     * of bytes required (excluding the terminating NUL). On success the
     * function returns the number of bytes written (excluding the terminating NUL).
     *
     * @param api  API context pointer created with ifc_api_new.
     * @param text NULL‑terminated UTF‑8 string to decode.
     * @param out  Caller-supplied buffer to receive the decoded text, or NULL.
     * @returns Number of bytes written or required; zero on error.
     */
    FFI_EXPORT size_t ifc_api_decode_text(const IfcAPI *api,
                                          const char *text,
                                          char *out);

    /**
     * Resets the cached IFC data for a model – useful when changing the geometry
     * of a model.
     *
     * @param api      API context pointer created with ifc_api_new.
     * @param model_id Model handle retrieved by OpenModel.
     * @returns A newly allocated string returned from the underlying decode
     *          operation, or NULL on error.  The caller is responsible for
     *          freeing the returned string.
     */
    FFI_EXPORT void ifc_api_reset_cache(const IfcAPI *api,
                                        uint32_t model_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WEB_IFC_API_H */
