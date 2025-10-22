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

#include <stddef.h>   /* size_t */
#include <stdint.h>   /* uint8_t, uint32_t, uint64_t */
#include <stdbool.h>  /* bool */
#include <time.h>     /* clock_gettime/timespec_get */
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
    namespace webifc { namespace manager { class ModelManager; } }
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define UNKNOWN 0
#define STRING 1
#define LABEL 2
#define ENUM 3
#define REAL 4
#define REF 5
#define EMPTY 6
#define SET_BEGIN 7
#define SET_END 8
#define LINE_END 9
#define INTEGER 10

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
typedef struct {
    const bool*     COORDINATE_TO_ORIGIN;            /* If true, translate model to the origin. */
    const int*      CIRCLE_SEGMENTS;                 /* Segments used to approximate circles. */
    const uint64_t* MEMORY_LIMIT;                    /* Max memory for IFC data in bytes. */
    const uint64_t* TAPE_SIZE;                       /* Internal buffer tape size (bytes/units). */
    const uint32_t* LINEWRITER_BUFFER;               /* Lines to buffer when writing IFC files. */
    const double*   TOLERANCE_PLANE_INTERSECTION;    /* Tolerance for plane intersection checks. */
    const double*   TOLERANCE_PLANE_DEVIATION;       /* Tolerance to consider a plane on a boundary. */
    const double*   TOLERANCE_BACK_DEVIATION_DISTANCE;/* Threshold to decide front/back of a plane. */
    const double*   TOLERANCE_INSIDE_OUTSIDE_PERIMETER;/* Tolerance for point-in-perimeter tests. */
    const double*   TOLERANCE_SCALAR_EQUALITY;       /* Tolerance for scalar equality comparisons. */
    const int*      PLANE_REFIT_ITERATIONS;          /* Iterations for refitting triangles to a plane. */
    const int*      BOOLEAN_UNION_THRESHOLD;         /* Min solids before performing boolean union. */
} LoaderSettings;

/* Define default values with uppercase names */
static const bool     DEFAULT_COORDINATE_TO_ORIGIN              = false;
static const int      DEFAULT_CIRCLE_SEGMENTS                   = 12;
static const uint64_t DEFAULT_MEMORY_LIMIT                      = 2147483648ULL;  /* 2 GiB */
static const uint64_t DEFAULT_TAPE_SIZE                         = 67108864ULL;    /* 64 MiB */
static const uint32_t DEFAULT_LINEWRITER_BUFFER                 = 10000;
static const double   DEFAULT_TOLERANCE_PLANE_INTERSECTION      = 1.0e-4;
static const double   DEFAULT_TOLERANCE_PLANE_DEVIATION         = 1.0e-4;
static const double   DEFAULT_TOLERANCE_BACK_DEVIATION_DISTANCE = 1.0e-4;
static const double   DEFAULT_TOLERANCE_INSIDE_OUTSIDE_PERIMETER= 1.0e-10;
static const double   DEFAULT_TOLERANCE_SCALAR_EQUALITY         = 1.0e-4;
static const int      DEFAULT_PLANE_REFIT_ITERATIONS            = 1;
static const int      DEFAULT_BOOLEAN_UNION_THRESHOLD           = 150;

typedef struct { double*   data; size_t len; } DoubleArray;
typedef struct { uint32_t* data; size_t len; } UInt32Array;
typedef struct { int*      data; size_t len; } IntArray;
typedef struct { char**    data; size_t len; } StringArray;

typedef struct { double x, y, z, w; } Color;

typedef struct {
    int     ID;
    int     type;
    void**  arguments;     /* any[]; your app decides element types */
    size_t  arguments_len;
} RawLineData;

typedef struct {
    Color   color;
    int     geometryExpressID;
    double* flatTransformation;      /* e.g., 16 elements for 4x4 */
    size_t  flatTransformation_len;
} PlacedGeometry;

typedef struct {
    PlacedGeometry* data;
    size_t          size;
} PlacedGeometryVector;

typedef struct FlatMesh {
    PlacedGeometryVector geometries;
    int                  expressID;
    void (*destroy)(struct FlatMesh* self);  /* optional */
} FlatMesh;

/* Point with optional z (NULL => 2D) */
typedef struct {
    double x, y;
    const double* z;  /* optional */
} Point;

typedef struct { Point*  data; size_t len; } PointArray;
typedef struct { double* data; size_t len; } NumberArray;
typedef struct { char**  data; size_t len; } UserDataArray;

typedef struct Curve {
    PointArray    points;       /* Array<Point> */
    UserDataArray userData;     /* Array<string> */
    NumberArray   arcSegments;  /* Array<number> */
} Curve;

typedef struct { Curve* data; size_t len; } CurveArray;

/* Forward for recursive Profile */
struct Profile;
typedef struct { struct Profile* data; size_t len; } ProfileArray;

typedef struct Profile {
    Curve        curve;
    CurveArray   holes;     /* Array<Curve> */
    ProfileArray profiles;  /* Array<Profile> */
    bool         isConvex;
    bool         isComposite;
} Profile;

typedef struct {
    CurveArray curves;   /* Array<Curve> */
    IntArray   expressID;/* Array<number> */
} CrossSection;

typedef struct { CurveArray curves; } AlignmentSegment;

typedef struct {
    DoubleArray     FlatCoordinationMatrix; /* Array<number> */
    AlignmentSegment Horizontal;
    AlignmentSegment Vertical;
    AlignmentSegment Absolute;
} AlignmentData;

typedef struct {
    Profile    profile;
    CurveArray axis;           /* Array<Curve> */
    double     profileRadius;
} SweptDiskSolid;

/* ========== Geometry buffers ========== */
typedef struct {
    DoubleArray fvertexData;   /* vertex floats (stored as double here) */
    UInt32Array indexData;     /* indices */
} Buffers;

/* ========== Interfaces with methods (function-pointer style) ========== */
typedef struct IfcGeometry {
    size_t (*GetVertexData)(struct IfcGeometry* self);
    size_t (*GetVertexDataSize)(struct IfcGeometry* self);
    size_t (*GetIndexData)(struct IfcGeometry* self);
    size_t (*GetIndexDataSize)(struct IfcGeometry* self);
    SweptDiskSolid (*GetSweptDiskSolid)(struct IfcGeometry* self);
    void (*destroy)(struct IfcGeometry* self);
    void* user;
} IfcGeometry;

typedef struct AABB {
    Buffers (*GetBuffers)(struct AABB* self);
    void    (*SetValues)(struct AABB* self,
                         double minX, double minY, double minZ,
                         double maxX, double maxY, double maxZ);
    void* user;
} AABB;

typedef struct Extrusion {
    Buffers (*GetBuffers)(struct Extrusion* self);
    void    (*SetValues)(struct Extrusion* self,
                         const double* profile, size_t profile_len,
                         const double* dir,     size_t dir_len,
                         double len,
                         const double* cuttingPlaneNormal, size_t cuttingPlaneNormal_len,
                         const double* cuttingPlanePos,    size_t cuttingPlanePos_len,
                         bool cap);
    void    (*SetHoles)(struct Extrusion* self, const double* profile, size_t profile_len);
    void    (*ClearHoles)(struct Extrusion* self);
    void* user;
} Extrusion;

typedef struct Sweep {
    Buffers (*GetBuffers)(struct Sweep* self);
    void    (*SetValues)(struct Sweep* self,
                         double scaling,
                         const bool* closed,                /* optional */
                         const double* profile,   size_t profile_len,
                         const double* directrix, size_t directrix_len,
                         const double* initialNormal, size_t initialNormal_len, /* optional */
                         const bool* rotate90,            /* optional */
                         const bool* optimize);           /* optional */
    void* user;
} Sweep;

typedef struct CircularSweep {
    Buffers (*GetBuffers)(struct CircularSweep* self);
    void    (*SetValues)(struct CircularSweep* self,
                         double scaling,
                         const bool* closed,               /* optional */
                         const double* profile, size_t profile_len,
                         double radius,
                         const double* directrix, size_t directrix_len,
                         const double* initialNormal, size_t initialNormal_len, /* optional */
                         const bool* rotate90);           /* optional */
    void* user;
} CircularSweep;

typedef struct Revolution {
    Buffers (*GetBuffers)(struct Revolution* self);
    void    (*SetValues)(struct Revolution* self,
                         const double* profile,  size_t profile_len,
                         const double* transform,size_t transform_len,
                         double startDegrees,
                         double endDegrees,
                         int    numRots);
    void* user;
} Revolution;

typedef struct CylindricalRevolve {
    Buffers (*GetBuffers)(struct CylindricalRevolve* self);
    void    (*SetValues)(struct CylindricalRevolve* self,
                         const double* transform, size_t transform_len,
                         double startDegrees, double endDegrees,
                         double minZ, double maxZ,
                         int    numRots,
                         double radius);
    void* user;
} CylindricalRevolve;

typedef struct Parabola {
    Buffers (*GetBuffers)(struct Parabola* self);
    void    (*SetValues)(struct Parabola* self,
                         int    segments,
                         double startPointX, double startPointY, double startPointZ,
                         double horizontalLength,
                         double startHeight,
                         double startGradient,
                         double endGradient);
    void* user;
} Parabola;

typedef struct Clothoid {
    Buffers (*GetBuffers)(struct Clothoid* self);
    void    (*SetValues)(struct Clothoid* self,
                         int    segments,
                         double startPointX, double startPointY, double startPointZ,
                         double ifcStartDirection,
                         double StartRadiusOfCurvature,
                         double EndRadiusOfCurvature,
                         double SegmentLength);
    void* user;
} Clothoid;

typedef struct Arc {
    Buffers (*GetBuffers)(struct Arc* self);
    void    (*SetValues)(struct Arc* self,
                         double radiusX, double radiusY,
                         int    numSegments,
                         const double* placement, size_t placement_len,
                         const double* startRad,          /* optional */
                         const double* endRad,            /* optional */
                         const bool*   swap,              /* optional */
                         const bool*   normalToCenterEnding); /* optional */
    void* user;
} Arc;

/* Second TS “Alignment” with methods; renamed to avoid collision with data holder */
typedef struct AlignmentOp {
    Buffers (*GetBuffers)(struct AlignmentOp* self);
    void    (*SetValues)(struct AlignmentOp* self,
                         const double* horizontal, size_t horizontal_len,
                         const double* vertical,   size_t vertical_len);
    void* user;
} AlignmentOp;

typedef struct BooleanOperator {
    Buffers (*GetBuffers)(struct BooleanOperator* self);
    void    (*SetValues)(struct BooleanOperator* self,
                         const double* triangles, size_t triangles_len,
                         const char*   type_); /* "union" / "diff" / "intersect" */
    void    (*SetSecond)(struct BooleanOperator* self,
                         const double* triangles, size_t triangles_len);
    void    (*clear)(struct BooleanOperator* self);
    void* user;
} BooleanOperator;

typedef struct ProfileSection {
    Buffers (*GetBuffers)(struct ProfileSection* self);
    void    (*SetValues)(struct ProfileSection* self,
                         int    pType,
                         double width,
                         double depth,
                         double webThickness,
                         double flangeThickness,
                         const bool* hasFillet,     /* optional */
                         double filletRadius,
                         double radius,
                         double slope,
                         int    circleSegments,
                         const double* placement, size_t placement_len);
    void* user;
} ProfileSection;

/* ========== Meta and model ========== */
typedef struct {
    int         typeID;
    const char* typeName;  /* not owned */
} IfcType;

typedef struct {
    const char* schema;         /* required */
    const char* name;           /* optional (NULL) */
    StringArray description;    /* optional (len==0 or NULL) */
    StringArray authors;        /* optional */
    StringArray organizations;  /* optional */
    const char* authorization;  /* optional (NULL) */
} NewIfcModel;

/* ========== I/O callbacks ========== */
typedef struct { uint8_t* data; size_t len; } ByteArray;
typedef ByteArray (*ModelLoadCallback)(size_t offset, size_t size, void* user);
typedef void (*ModelSaveCallback)(const uint8_t* data, size_t len, void* user);
/* (path, prefix) -> absolute path/URL; return UTF-8 C string (policy up to app) */
// typedef const char* (*LocateFileHandlerFn)(const char* path, const char* prefix, void* user);

/* ========== ms(): milliseconds since Unix epoch (like TS ms) ========== */
static inline uint64_t ms(void) {
#if defined(_WIN32)
    FILETIME ft;
    ULARGE_INTEGER u;
    const uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL; /* 1601->1970 */
    GetSystemTimeAsFileTime(&ft);
    u.LowPart  = ft.dwLowDateTime;
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

/* Main API structure storing library state. */
typedef struct {
    int    *model_schema_list;
    char  **model_schema_name_list;
    size_t  model_schema_count;
    int   **deleted_lines;
    size_t  deleted_lines_count;
    void   *properties;
#ifdef __cplusplus
    webifc::manager::ModelManager* manager;
#else
    void* manager;
#endif
} IfcAPI;

/* Create a new API object.  Memory is allocated with malloc. */
FFI_EXPORT IfcAPI *ifc_api_new(void);

/* Free an API object and any internal allocations. */
FFI_EXPORT void ifc_api_free(IfcAPI *api);

/* Initialize the API.  In this stub implementation this always
 * succeeds and returns 0.  A custom locate file handler may be
 * supplied as a pointer (unused here). */
FFI_EXPORT int ifc_api_init(IfcAPI *api);

static int find_schema_index(const char* schemaName) {
  for (size_t i = 0; i < SCHEMA_NAME_ROWS; ++i) {
    for (size_t j = 0; j < SCHEMA_NAME_INDEX[i].len; ++j) {
      const char* name = SCHEMA_NAME_DATA[SCHEMA_NAME_INDEX[i].off + j];
      if (name && strcmp(name, schemaName) == 0) return (int)i;
    }
  }
  return -1;
}

static inline LoaderSettings ifc_api_create_settings(const LoaderSettings* in) {
    LoaderSettings s;
    /* Booleans / ints / doubles all use pointer fields; choose caller’s pointer or default’s address */
    s.COORDINATE_TO_ORIGIN               = (in && in->COORDINATE_TO_ORIGIN)               ? in->COORDINATE_TO_ORIGIN               : &DEFAULT_COORDINATE_TO_ORIGIN;
    s.CIRCLE_SEGMENTS                    = (in && in->CIRCLE_SEGMENTS)                    ? in->CIRCLE_SEGMENTS                    : &DEFAULT_CIRCLE_SEGMENTS;
    s.MEMORY_LIMIT                       = (in && in->MEMORY_LIMIT)                       ? in->MEMORY_LIMIT                       : &DEFAULT_MEMORY_LIMIT;
    s.TAPE_SIZE                          = (in && in->TAPE_SIZE)                          ? in->TAPE_SIZE                          : &DEFAULT_TAPE_SIZE;
    s.LINEWRITER_BUFFER                  = (in && in->LINEWRITER_BUFFER)                  ? in->LINEWRITER_BUFFER                  : &DEFAULT_LINEWRITER_BUFFER;
    s.TOLERANCE_PLANE_INTERSECTION       = (in && in->TOLERANCE_PLANE_INTERSECTION)       ? in->TOLERANCE_PLANE_INTERSECTION       : &DEFAULT_TOLERANCE_PLANE_INTERSECTION;
    s.TOLERANCE_PLANE_DEVIATION          = (in && in->TOLERANCE_PLANE_DEVIATION)          ? in->TOLERANCE_PLANE_DEVIATION          : &DEFAULT_TOLERANCE_PLANE_DEVIATION;
    s.TOLERANCE_BACK_DEVIATION_DISTANCE  = (in && in->TOLERANCE_BACK_DEVIATION_DISTANCE)  ? in->TOLERANCE_BACK_DEVIATION_DISTANCE  : &DEFAULT_TOLERANCE_BACK_DEVIATION_DISTANCE;
    s.TOLERANCE_INSIDE_OUTSIDE_PERIMETER = (in && in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER) ? in->TOLERANCE_INSIDE_OUTSIDE_PERIMETER : &DEFAULT_TOLERANCE_INSIDE_OUTSIDE_PERIMETER;
    s.TOLERANCE_SCALAR_EQUALITY          = (in && in->TOLERANCE_SCALAR_EQUALITY)          ? in->TOLERANCE_SCALAR_EQUALITY          : &DEFAULT_TOLERANCE_SCALAR_EQUALITY;
    s.PLANE_REFIT_ITERATIONS             = (in && in->PLANE_REFIT_ITERATIONS)             ? in->PLANE_REFIT_ITERATIONS             : &DEFAULT_PLANE_REFIT_ITERATIONS;
    s.BOOLEAN_UNION_THRESHOLD            = (in && in->BOOLEAN_UNION_THRESHOLD)            ? in->BOOLEAN_UNION_THRESHOLD            : &DEFAULT_BOOLEAN_UNION_THRESHOLD;
    return s;
}

/* Open multiple models from byte buffers.  Returns an array of model
 * IDs allocated with malloc and stores its length in out_count.  The
 * returned array and its contents should be freed by the caller. */
FFI_EXPORT int *ifc_api_open_models(IfcAPI *api,
                        const ByteArray* data_sets, /* array of ByteArray */
                        size_t num_data_sets,
                        const LoaderSettings *settings,
                        size_t *out_count);

/* Open a single model from a buffer.  Returns a model ID on success
 * or -1 on failure. */
FFI_EXPORT int ifc_api_open_model(IfcAPI *api,
                        const ByteArray data,
                        const LoaderSettings *settings);

/* Open a model by streaming bytes using a user provided callback. */
FFI_EXPORT int ifc_api_open_model_from_callback(IfcAPI *api,
                                      ModelLoadCallback callback,
                                      void *load_cb_user_data,
                                      const LoaderSettings *settings);

/* Retrieve the schema name for a given model ID.  Returns NULL if
 * unknown.  The returned string is owned by the API. */
FFI_EXPORT const char *ifc_api_get_model_schema(const IfcAPI *api, int model_id);

/* Create a new model.  Returns a model ID on success or -1 on error. */
FFI_EXPORT int ifc_api_create_model(IfcAPI *api,
                          const NewIfcModel *model,
                          const LoaderSettings *settings);

/* Save a model to a contiguous buffer.  Returns a malloc'ed buffer
 * and stores its size in out_size.  The caller must free the buffer. */
FFI_EXPORT uint8_t *ifc_api_save_model(const IfcAPI *api,
                             int model_id,
                             size_t *out_size);

/* Save a model by streaming bytes via a callback. */
FFI_EXPORT void ifc_api_save_model_to_callback(const IfcAPI *api,
                                     int model_id,
                                     ModelSaveCallback save_cb,
                                     void *save_cb_user_data);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WEB_IFC_API_H */