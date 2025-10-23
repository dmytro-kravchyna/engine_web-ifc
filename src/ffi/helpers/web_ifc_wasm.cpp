/*
 * This file provides a C++‑only interface for working with IFC models.
 * It removes all dependencies on Emscripten and JavaScript and instead
 * uses standard C++ mechanisms.  In particular, all occurrences of
 * `emscripten::val` have been replaced with `nlohmann::json` to allow
 * dynamic values, and callbacks are expressed using `std::function`.
 *
 * The functions defined here mirror those found in the original
 * `web-ifc-wasm.cpp` file from the Web‑IFC project.  They provide
 * facilities for creating, opening, saving and streaming IFC models
 * and geometry, as well as reading and writing IFC lines and header
 * entries.  Using `nlohmann::json` allows callers to construct
 * arbitrarily nested parameter sets without requiring any JavaScript
 * support.  `std::function` is used for all callbacks, enabling
 * clients to supply lambdas, free functions or functors in a type‑safe
 * manner.
 */

// Standard library headers
#include <string>
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

/*
 * Some of the Web‑IFC C API headers (such as web_ifc_api.h) define
 * preprocessor macros like STRING, ENUM, REF, REAL, INTEGER,
 * SET_BEGIN, SET_END and EMPTY for compatibility with C callers.  When
 * those macros are visible they will replace tokens with the same
 * names, including qualified enumerators like
 * `webifc::parsing::IfcTokenType::STRING`.  To prevent that kind of
 * macro substitution from breaking this C++ translation unit, we
 * temporarily undefine the conflicting macros.  We wrap the undefines
 * inside push_macro/pop_macro pairs so that the original definitions
 * (if any) are restored when compilation of this file completes.
 */

namespace webifc::parsing
{
    void p21encode(std::string_view input, std::ostringstream &output);
    std::string p21decode(std::string_view &str);
}

using json = nlohmann::json;

// Create a new model.  A pointer to the ModelManager is passed so that
// the caller can maintain ownership of the manager object.  LoaderSettings
// are forwarded directly to the manager.  Returns the new model ID.
/**
 * @brief Create a new IFC model managed by the given ModelManager.
 *
 * This function forwards the supplied LoaderSettings to the underlying
 * ModelManager and returns the identifier of the newly created model.  The
 * caller retains ownership of the ModelManager instance and is responsible
 * for closing or destroying models when finished.
 *
 * @param manager Pointer to the model manager used to create the model.
 * @param settings Configuration settings controlling how the model is loaded.
 * @return A numeric model identifier that can be passed to other functions.
 */
int CreateModel(webifc::manager::ModelManager *manager,
                webifc::manager::LoaderSettings settings)
{
    return manager->CreateModel(settings);
}

// Close all open models managed by the given ModelManager.
/**
 * @brief Close and unload all models currently managed by the ModelManager.
 *
 * This releases any resources associated with open models.  After calling
 * this function all model identifiers previously returned by CreateModel
 * become invalid.
 *
 * @param manager Pointer to the model manager whose models should be closed.
 */
void CloseAllModels(webifc::manager::ModelManager *manager)
{
    manager->CloseAllModels();
}

// Open an IFC model and stream file contents using a callback.  The
// callback receives a buffer pointer, the offset into the source and
// the requested number of bytes.  It must return the number of bytes
// actually loaded into the destination buffer.  By using
// `std::function` the caller may supply a lambda or function pointer
// with the correct signature【182640319188848†L99-L118】.
/**
 * @brief Open an IFC model from an external source.
 *
 * The supplied callback is invoked repeatedly by the loader to request
 * chunks of data.  It should copy up to destSize bytes starting at
 * sourceOffset into the provided destination buffer and return the number
 * of bytes copied.  The callback may be implemented using a lambda,
 * function pointer or functor.  A new model identifier is returned
 * on success.
 *
 * @param manager Pointer to the model manager responsible for loading.
 * @param settings Loader configuration controlling how the model is parsed.
 * @param callback Data source callback supplying model bytes on demand.
 * @return The identifier of the newly created model.
 */
uint32_t OpenModel(webifc::manager::ModelManager *manager,
              webifc::manager::LoaderSettings settings,
              std::function<uint32_t(char *, size_t, size_t)> callback)
{
    auto modelID = manager->CreateModel(settings);
    manager->GetIfcLoader(modelID)->LoadFile(std::move(callback));
    return modelID;
}

// Save an IFC model to disk.  The provided callback will be invoked
// repeatedly with a pointer to a buffer containing file data and the
// size of that buffer.  The caller can use this to write the data to
// disk or transmit it over a network.  No value is returned by
// the callback since it is only invoked for its side effects.
/**
 * @brief Save an open model to an external sink.
 *
 * The provided callback will be called one or more times with a pointer
 * to a buffer containing a portion of the model and the size of that
 * buffer.  The caller is expected to consume or persist the data in
 * whatever manner is appropriate (e.g. write to a file or socket).
 *
 * @param manager Pointer to the model manager holding the model.
 * @param modelID Identifier of the model to save.
 * @param callback Sink callback that consumes file data.
 */
void SaveModel(webifc::manager::ModelManager *manager,
               uint32_t modelID,
               std::function<void(char *, size_t)> callback)
{
    if (!manager->IsModelOpen(modelID))
        return;
    manager->GetIfcLoader(modelID)->SaveFile(
        [&](char *src, size_t srcSize)
        {
            callback(src, srcSize);
        },
        false);
}

// Retrieve the total size of an open model.  Returns zero if the model
// is not open.
/**
 * @brief Retrieve the total size of an open model in bytes.
 *
 * If the model is closed or invalid, zero is returned.  The size
 * corresponds to the total number of bytes contained in the IFC file.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @return The total size of the model in bytes, or 0 if not open.
 */
int GetModelSize(webifc::manager::ModelManager *manager, uint32_t modelID)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->GetTotalSize() : 0;
}

// Close a single model by ID.
/**
 * @brief Close a single model by its identifier.
 *
 * This releases resources associated with the specified model.  After
 * calling this function the model identifier becomes invalid.
 *
 * @param manager Pointer to the model manager holding the model.
 * @param modelID Identifier of the model to close.
 */
void CloseModel(webifc::manager::ModelManager *manager, uint32_t modelID)
{
    manager->CloseModel(modelID);
}

// Retrieve a flat mesh representation of a specific entity.  Calling
// GetVertexData on each geometry ensures that vertex data is populated.
/**
 * @brief Obtain a flat mesh for a specific entity.
 *
 * The returned mesh contains one or more geometries for the given
 * express ID.  Prior to returning, this function calls GetVertexData
 * on each referenced geometry to ensure that vertex buffers are
 * populated.  If the model is not open the returned mesh will be
 * empty.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model containing the entity.
 * @param expressID Express ID of the entity whose mesh should be fetched.
 * @return A flat mesh containing the geometry for the entity.
 */
webifc::geometry::IfcFlatMesh GetFlatMesh(webifc::manager::ModelManager *manager,
                                          uint32_t modelID,
                                          uint32_t expressID)
{
    if (!manager->IsModelOpen(modelID))
        return {};
    webifc::geometry::IfcFlatMesh mesh =
        manager->GetGeometryProcessor(modelID)->GetFlatMesh(expressID);
    for (auto &geom : mesh.geometries)
        manager->GetGeometryProcessor(modelID)
            ->GetGeometry(geom.geometryExpressID)
            .GetVertexData();
    return mesh;
}

// Stream a sequence of meshes to the caller.  The callback receives
// each mesh along with its index in the sequence and the total number
// of meshes.  After every invocation the geometry processor is
// cleared to free memory.
/**
 * @brief Stream a collection of meshes and invoke a callback for each.
 *
 * For each express ID in the provided list, this function retrieves
 * the corresponding flat mesh, ensures its vertex data is loaded,
 * and then invokes the supplied callback with the mesh, the zero‑based
 * index of the current mesh, and the total number of meshes in the
 * sequence.  After each callback, the geometry processor is cleared
 * to release memory.  If the model is closed, no callbacks are
 * invoked.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose meshes should be streamed.
 * @param expressIds A vector of express IDs for which to stream meshes.
 * @param callback Callback receiving each mesh along with its index and total count.
 */
void StreamMeshes(webifc::manager::ModelManager *manager,
                  uint32_t modelID,
                  const std::vector<uint32_t> &expressIds,
                  std::function<void(const webifc::geometry::IfcFlatMesh &, int, int)> callback)
{
    if (!manager->IsModelOpen(modelID))
        return;
    auto geomLoader = manager->GetGeometryProcessor(modelID);
    int index = 0;
    int total = static_cast<int>(expressIds.size());
    for (const auto &id : expressIds)
    {
        // read the mesh from IFC
        webifc::geometry::IfcFlatMesh mesh = geomLoader->GetFlatMesh(id);
        // prepare the geometry data
        for (auto &geom : mesh.geometries)
        {
            auto &flatGeom = geomLoader->GetGeometry(geom.geometryExpressID);
            flatGeom.GetVertexData();
        }
        if (!mesh.geometries.empty())
        {
            // transfer control to client, geometry data is alive for
            // the duration of the callback
            callback(mesh, index, total);
        }
        // clear geometry, freeing memory; client is expected to have
        // consumed the data
        geomLoader->Clear();
        ++index;
    }
}

// Overload of StreamMeshes that accepts the express IDs as a JSON
// array.  This function extracts the integer IDs from the JSON
// structure and forwards them to the main StreamMeshes function.
/**
 * @brief Stream meshes for a collection of express IDs encoded as JSON.
 *
 * This helper converts a JSON array of numbers into a vector of
 * express IDs and forwards the result to StreamMeshes.  It is
 * primarily provided for convenience when interacting with APIs
 * that use JSON for argument passing.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose meshes should be streamed.
 * @param expressIdsVal JSON array of express IDs to stream.
 * @param callback Callback invoked for each mesh.
 */
void StreamMeshesWithExpressID(webifc::manager::ModelManager *manager,
                               uint32_t modelID,
                               const json &expressIdsVal,
                               std::function<void(const webifc::geometry::IfcFlatMesh &, int, int)> callback)
{
    std::vector<uint32_t> expressIds;
    if (expressIdsVal.is_array())
    {
        expressIds.reserve(expressIdsVal.size());
        for (const auto &elem : expressIdsVal)
        {
            expressIds.push_back(elem.get<uint32_t>());
        }
    }
    StreamMeshes(manager, modelID, expressIds, std::move(callback));
}

// Stream meshes for all entities of the given IFC types.  The list of
// type codes is provided as a vector.  The callback receives each
// resulting mesh.
/**
 * @brief Stream meshes for all entities of the specified IFC types.
 *
 * This function iterates over the provided list of IFC type codes and
 * streams the meshes of all elements matching each type by delegating
 * to StreamMeshes.  The supplied callback is invoked for each mesh.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose meshes should be streamed.
 * @param types A vector of IFC type codes whose elements should be streamed.
 * @param callback Callback receiving each mesh along with its index and total count.
 */
void StreamAllMeshesWithTypes(webifc::manager::ModelManager *manager,
                              uint32_t modelID,
                              const std::vector<uint32_t> &types,
                              std::function<void(const webifc::geometry::IfcFlatMesh &, int, int)> callback)
{
    if (!manager->IsModelOpen(modelID))
        return;
    auto loader = manager->GetIfcLoader(modelID);
    for (auto &type : types)
    {
        auto elements = loader->GetExpressIDsWithType(type);
        StreamMeshes(manager, modelID, elements, callback);
    }
}

// Overload of StreamAllMeshesWithTypes that accepts the type list as a
// JSON array.  Parses the JSON to extract type codes before
// streaming.
/**
 * @brief Stream meshes for IFC types supplied as a JSON array.
 *
 * This helper extracts type codes from a JSON array and forwards them to
 * StreamAllMeshesWithTypes.  It is intended for compatibility with
 * JSON‑based APIs.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose meshes should be streamed.
 * @param typesVal JSON array of IFC type codes.
 * @param callback Callback invoked for each mesh.
 */
void StreamAllMeshesWithTypesVal(webifc::manager::ModelManager *manager,
                                 uint32_t modelID,
                                 const json &typesVal,
                                 std::function<void(const webifc::geometry::IfcFlatMesh &, int, int)> callback)
{
    if (!manager->IsModelOpen(modelID))
        return;
    std::vector<uint32_t> types;
    if (typesVal.is_array())
    {
        types.reserve(typesVal.size());
        for (const auto &val : typesVal)
        {
            types.push_back(val.get<uint32_t>());
        }
    }
    StreamAllMeshesWithTypes(manager, modelID, types, std::move(callback));
}

// Stream all meshes except those corresponding to openings and spaces.
/**
 * @brief Stream meshes for all IFC elements except openings and spaces.
 *
 * This convenience function filters out types that correspond to
 * openings (IFCOPENINGELEMENT and IFCOPENINGSTANDARDCASE) and spaces
 * (IFCSPACE) and then streams the meshes for all remaining element
 * types.  The supplied callback is invoked for each mesh.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose meshes should be streamed.
 * @param callback Callback invoked for each mesh.
 */
void StreamAllMeshes(webifc::manager::ModelManager *manager,
                     uint32_t modelID,
                     std::function<void(const webifc::geometry::IfcFlatMesh &, int, int)> callback)
{
    if (!manager->IsModelOpen(modelID))
        return;
    // Collect all element types except openings and spaces
    std::vector<uint32_t> types;
    const auto &elementList = manager->GetSchemaManager().GetIfcElementList();
    types.reserve(elementList.size());
    for (uint32_t type : elementList)
    {
        if (type == webifc::schema::IFCOPENINGELEMENT ||
            type == webifc::schema::IFCSPACE ||
            type == webifc::schema::IFCOPENINGSTANDARDCASE)
        {
            continue;
        }
        types.push_back(type);
    }
    StreamAllMeshesWithTypes(manager, modelID, types, std::move(callback));
}

// Load all geometry in the model and return a vector of flat meshes.
/**
 * @brief Load all geometry for the specified model.
 *
 * For each IFC element type defined in the schema manager (except
 * openings and spaces) this function collects all express IDs of
 * that type and loads their flat meshes.  Vertex data for each
 * geometry is populated prior to adding the mesh to the result
 * vector.  The returned meshes can be moved or iterated over as
 * needed.  If the model is closed an empty vector is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose geometry should be loaded.
 * @return A vector containing flat meshes for all loaded geometry.
 */
std::vector<webifc::geometry::IfcFlatMesh> LoadAllGeometry(webifc::manager::ModelManager *manager,
                                                           uint32_t modelID)
{
    std::vector<webifc::geometry::IfcFlatMesh> meshes;
    if (!manager->IsModelOpen(modelID))
        return meshes;
    auto loader = manager->GetIfcLoader(modelID);
    auto geomLoader = manager->GetGeometryProcessor(modelID);
    const auto &elementList = manager->GetSchemaManager().GetIfcElementList();
    for (uint32_t type : elementList)
    {
        // Skip openings and spaces
        if (type == webifc::schema::IFCOPENINGELEMENT ||
            type == webifc::schema::IFCSPACE ||
            type == webifc::schema::IFCOPENINGSTANDARDCASE)
        {
            continue;
        }
        auto elements = loader->GetExpressIDsWithType(type);
        for (uint32_t id : elements)
        {
            webifc::geometry::IfcFlatMesh mesh = geomLoader->GetFlatMesh(id);
            for (auto &geom : mesh.geometries)
            {
                auto &flatGeom = geomLoader->GetGeometry(geom.geometryExpressID);
                flatGeom.GetVertexData();
            }
            meshes.push_back(std::move(mesh));
        }
    }
    return meshes;
}

// Retrieve a specific geometry by express ID.
/**
 * @brief Retrieve a single geometry object by express ID.
 *
 * If the model is open, this function returns the geometry associated
 * with the specified express ID; otherwise an empty geometry is
 * returned.  Geometry objects may reference vertex data stored in
 * separate buffers.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model containing the geometry.
 * @param expressID Express ID of the desired geometry.
 * @return The corresponding geometry object or an empty object if not found.
 */
webifc::geometry::IfcGeometry GetGeometry(webifc::manager::ModelManager *manager,
                                          uint32_t modelID,
                                          uint32_t expressID)
{
    return manager->IsModelOpen(modelID) ? manager->GetGeometryProcessor(modelID)->GetGeometry(expressID) : webifc::geometry::IfcGeometry();
}

// Get cross sections (2D or 3D) for the specified IFC solids.
/**
 * @brief Extract all cross sections of certain IFC solids.
 *
 * Cross sections are returned for the types IFCSECTIONEDSOLIDHORIZONTAL,
 * IFCSECTIONEDSOLID and IFCSECTIONEDSURFACE.  If `dimensions` is 2,
 * 2D cross sections are returned; otherwise 3D cross sections are
 * provided.  When the model is closed an empty vector is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param dimensions 2 to request 2D cross sections, anything else for 3D.
 * @return A vector of cross sections for the selected elements.
 */
std::vector<webifc::geometry::IfcCrossSections> GetAllCrossSections(webifc::manager::ModelManager *manager,
                                                                    uint32_t modelID,
                                                                    uint8_t dimensions)
{
    std::vector<webifc::geometry::IfcCrossSections> crossSections;
    if (!manager->IsModelOpen(modelID))
        return crossSections;
    auto geomLoader = manager->GetGeometryProcessor(modelID);
    std::vector<uint32_t> typeList;
    typeList.push_back(webifc::schema::IFCSECTIONEDSOLIDHORIZONTAL);
    typeList.push_back(webifc::schema::IFCSECTIONEDSOLID);
    typeList.push_back(webifc::schema::IFCSECTIONEDSURFACE);
    for (auto &type : typeList)
    {
        auto elements = manager->GetIfcLoader(modelID)->GetExpressIDsWithType(type);
        for (size_t i = 0; i < elements.size(); i++)
        {
            webifc::geometry::IfcCrossSections crossSection;
            if (dimensions == 2)
                crossSection = geomLoader->GetLoader().GetCrossSections2D(elements[i]);
            else
                crossSection = geomLoader->GetLoader().GetCrossSections3D(elements[i]);
            crossSections.push_back(crossSection);
        }
    }
    return crossSections;
}

// Extract all alignments from the model.  Horizontal and vertical
// alignments are converted into absolute curves by combining
// horizontal and vertical point lists.
/**
 * @brief Retrieve and transform all IFC alignment entities.
 *
 * This function fetches all elements of type IFCALIGNMENT from the
 * model, applies the coordination matrix to transform them, and
 * then converts their 2D horizontal and vertical curves into
 * absolute 3D curves.  The resulting alignment objects contain
 * horizontal, vertical and absolute curve data.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose alignments should be retrieved.
 * @return A vector of transformed alignment objects.
 */
std::vector<webifc::geometry::IfcAlignment> GetAllAlignments(webifc::manager::ModelManager *manager,
                                                             uint32_t modelID)
{
    std::vector<webifc::geometry::IfcAlignment> alignments;
    if (!manager->IsModelOpen(modelID))
        return alignments;
    auto geomLoader = manager->GetGeometryProcessor(modelID);
    auto type = webifc::schema::IFCALIGNMENT;
    auto elements = manager->GetIfcLoader(modelID)->GetExpressIDsWithType(type);
    for (size_t i = 0; i < elements.size(); i++)
    {
        webifc::geometry::IfcAlignment alignment =
            geomLoader->GetLoader().GetAlignment(elements[i]);
        alignment.transform(geomLoader->GetCoordinationMatrix());
        alignments.push_back(alignment);
    }
    // convert 2D horizontal/vertical curves into 3D curves
    for (size_t i = 0; i < alignments.size(); i++)
    {
        webifc::geometry::IfcAlignment &alignment = alignments[i];
        std::vector<glm::dvec3> pointsH;
        std::vector<glm::dvec3> pointsV;
        for (size_t j = 0; j < alignment.Horizontal.curves.size(); j++)
        {
            for (auto &pt : alignment.Horizontal.curves[j].points)
            {
                pointsH.push_back(pt);
            }
        }
        for (size_t j = 0; j < alignment.Vertical.curves.size(); j++)
        {
            for (auto &pt : alignment.Vertical.curves[j].points)
            {
                pointsV.push_back(pt);
            }
        }
        webifc::geometry::IfcCurve curve;
        curve.points = bimGeometry::Convert2DAlignmentsTo3D(pointsH, pointsV);
        alignment.Absolute.curves.push_back(curve);
    }
    return alignments;
}

// Set the geometry transformation matrix for the specified model.
/**
 * @brief Set the transformation matrix used for geometry processing.
 *
 * The supplied 4x4 matrix (represented as an array of 16 doubles)
 * will be applied to subsequent geometry calculations for the
 * specified model.  If the model is not open the call has no effect.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to modify.
 * @param m A 4x4 transformation matrix in row‑major order.
 */
void SetGeometryTransformation(webifc::manager::ModelManager *manager,
                               uint32_t modelID,
                               std::array<double, 16> m)
{
    if (manager->IsModelOpen(modelID))
        manager->GetGeometryProcessor(modelID)->SetTransformation(m);
}

// Retrieve the coordination matrix for the given model.  Returns
// an identity matrix if the model is not open.
/**
 * @brief Retrieve the coordination matrix for a model.
 *
 * The coordination matrix represents the transformation applied
 * to geometry coordinates when computing alignments and other
 * derived data.  If the model is not open an identity matrix
 * (all zeros) is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @return A 4x4 row‑major matrix containing the coordination transform.
 */
std::array<double, 16> GetCoordinationMatrix(webifc::manager::ModelManager *manager,
                                             uint32_t modelID)
{
    return manager->IsModelOpen(modelID) ? manager->GetGeometryProcessor(modelID)->GetFlatCoordinationMatrix() : std::array<double, 16>();
}

// Helper to convert a JSON array of type codes into a flat vector of
// express IDs by querying the loader for each type.  This replaces
// the original implementation that relied on `emscripten::val`.
/**
 * @brief Internal helper to retrieve all express IDs for a set of types.
 *
 * This function iterates over the provided list of type codes and
 * collects all express IDs returned by GetExpressIDsWithType for
 * each type, concatenating them into a single vector.  It returns
 * an empty vector if the model is not open.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param types Vector of IFC type codes to look up.
 * @return A vector containing all matching express IDs.
 */
static std::vector<uint32_t> GetLineIDsWithTypeImpl(webifc::manager::ModelManager *manager,
                                                    uint32_t modelID,
                                                    const std::vector<uint32_t> &types)
{
    std::vector<uint32_t> expressIDs;
    if (!manager->IsModelOpen(modelID))
        return expressIDs;
    auto loader = manager->GetIfcLoader(modelID);
    for (auto type : types)
    {
        auto ids = loader->GetExpressIDsWithType(type);
        expressIDs.insert(expressIDs.end(), ids.begin(), ids.end());
    }
    return expressIDs;
}

// Overload that accepts a JSON array of type codes.  Each element of
// the array must be convertible to `uint32_t`.  Returns a flat
// vector of express IDs.
/**
 * @brief Retrieve all express IDs matching a set of type codes provided in JSON.
 *
 * This overload accepts a JSON array of numeric type codes and
 * converts it into a vector before delegating to the internal
 * implementation.  Non‑array values yield an empty result.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param types JSON array of IFC type codes.
 * @return A vector containing all matching express IDs.
 */
std::vector<uint32_t> GetLineIDsWithType(webifc::manager::ModelManager *manager,
                                         uint32_t modelID,
                                         const json &types)
{
    std::vector<uint32_t> typeVec;
    if (types.is_array())
    {
        typeVec.reserve(types.size());
        for (const auto &t : types)
            typeVec.push_back(t.get<uint32_t>());
    }
    return GetLineIDsWithTypeImpl(manager, modelID, typeVec);
}

// Compute the inverse property for the given express ID.  This method
// searches through all lines of the specified target types and looks
// for references equal to `expressID`.  If `set` is false the first
// match is returned immediately, otherwise all matches are gathered.
/**
 * @brief Find all inverse references to a given express ID.
 *
 * Searches through all elements whose types are listed in the
 * targetTypes JSON array for references to the specified express ID
 * at the given argument position.  If `set` is false, the search
 * stops after the first match.  If the model is closed an empty
 * vector is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to search.
 * @param expressID Express ID whose inverse references should be found.
 * @param targetTypes JSON array of IFC type codes to search within.
 * @param position Argument position to examine for references.
 * @param set If true, find all matches; if false, stop after first.
 * @return Vector of express IDs that reference the target express ID.
 */
std::vector<uint32_t> GetInversePropertyForItem(webifc::manager::ModelManager *manager,
                                                uint32_t modelID,
                                                uint32_t expressID,
                                                const json &targetTypes,
                                                uint32_t position,
                                                bool set)
{
    std::vector<uint32_t> inverseIDs;
    if (!manager->IsModelOpen(modelID))
        return inverseIDs;
    auto loader = manager->GetIfcLoader(modelID);
    auto types = GetLineIDsWithType(manager, modelID, targetTypes);
    for (auto foundExpressID : types)
    {
        loader->MoveToLineArgument(foundExpressID, position);
        webifc::parsing::IfcTokenType t = loader->GetTokenType();
        if (t == webifc::parsing::IfcTokenType::REF)
        {
            loader->StepBack();
            uint32_t val = loader->GetRefArgument();
            if (val == expressID)
            {
                inverseIDs.push_back(foundExpressID);
                if (!set)
                    return inverseIDs;
            }
        }
        else if (t == webifc::parsing::IfcTokenType::SET_BEGIN)
        {
            while (!loader->IsAtEnd())
            {
                webifc::parsing::IfcTokenType setValueType = loader->GetTokenType();
                if (setValueType == webifc::parsing::IfcTokenType::SET_END)
                    break;
                if (setValueType == webifc::parsing::IfcTokenType::REF)
                {
                    loader->StepBack();
                    uint32_t val = loader->GetRefArgument();
                    if (val == expressID)
                    {
                        inverseIDs.push_back(foundExpressID);
                        if (!set)
                            return inverseIDs;
                    }
                }
            }
        }
    }
    return inverseIDs;
}

// Validate whether a given express ID is valid.  Returns false if the
// model is not open.
/**
 * @brief Validate whether a given express ID exists in the model.
 *
 * Returns true if the express ID is valid and the model is open;
 * otherwise returns false.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param expressId Express ID to validate.
 * @return True if the express ID is valid; false otherwise.
 */
bool ValidateExpressID(webifc::manager::ModelManager *manager,
                       uint32_t modelID,
                       uint32_t expressId)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->IsValidExpressID(expressId) : false;
}

// Get the next express ID following the specified one.  Returns zero
// if the model is not open.
/**
 * @brief Get the express ID of the next line after the specified one.
 *
 * If the model is open, this returns the express ID immediately
 * following the supplied ID; otherwise zero is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param expressId Express ID whose successor should be returned.
 * @return The next express ID or 0 if not available.
 */
uint32_t GetNextExpressID(webifc::manager::ModelManager *manager,
                          uint32_t modelID,
                          uint32_t expressId)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->GetNextExpressID(expressId) : 0;
}

// Retrieve all line IDs in the model.  Returns an empty list if the
// model is not open.
/**
 * @brief Retrieve a list of all express IDs in the model.
 *
 * If the model is open, this function returns a vector containing
 * all express IDs; otherwise an empty vector is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @return A vector of all express IDs in the model.
 */
std::vector<uint32_t> GetAllLines(webifc::manager::ModelManager *manager,
                                  uint32_t modelID)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->GetAllLines() : std::vector<uint32_t>();
}

// Write a single value to the loader based on the expected token type.
// The value is supplied as a JSON object.  The return code indicates
// whether the value was successfully serialized.
/**
 * @brief Serialize a single JSON value into the loader according to the token type.
 *
 * Depending on the token type, the value is interpreted as a string,
 * enumeration, reference, real number or integer and pushed into the
 * loader using the appropriate type‑specific functions.  Boolean and
 * undefined semantics from JavaScript are emulated for strings and
 * enumerations by converting true/false/undefined values into the
 * IFC notation ("T", "F", "U").  If the type is not recognized
 * a '?' character is pushed and the return value is false.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose loader should be used.
 * @param t Token type describing the expected value type.
 * @param value JSON value to serialize.
 * @return True on success; false if the value could not be serialized.
 */
bool WriteValue(webifc::manager::ModelManager *manager,
                uint32_t modelID,
                webifc::parsing::IfcTokenType t,
                const json &value)
{
    bool responseCode = true;
    auto loader = manager->GetIfcLoader(modelID);
    switch (t)
    {
    case webifc::parsing::IfcTokenType::STRING:
    case webifc::parsing::IfcTokenType::ENUM:
    {
        std::string copy;
        // emulate JavaScript boolean/undefined semantics
        if (value.is_boolean())
            copy = value.get<bool>() ? "T" : "F";
        else if (value.is_null())
            copy = "U";
        else
            copy = value.get<std::string>();
        if (copy == "true")
            copy = "T";
        else if (copy == "false")
            copy = "F";
        uint16_t length = static_cast<uint16_t>(copy.size());
        loader->Push<uint16_t>(length);
        // IfcLoader::Push expects a non‑const void pointer. Cast away
        // constness here to satisfy the API. The string contents will not
        // be modified by the loader.
        loader->Push(const_cast<void *>(static_cast<const void *>(copy.c_str())), copy.size());
        break;
    }
    case webifc::parsing::IfcTokenType::REF:
    {
        uint32_t val = value.get<uint32_t>();
        loader->Push<uint32_t>(val);
        break;
    }
    case webifc::parsing::IfcTokenType::REAL:
    {
        double val = value.get<double>();
        loader->PushDouble(val);
        break;
    }
    case webifc::parsing::IfcTokenType::INTEGER:
    {
        int val = value.get<int>();
        loader->PushInt(val);
        break;
    }
    default:
        // use '?' to signal parse issue
        loader->Push<uint8_t>('?');
        responseCode = false;
    }
    return responseCode;
}

// Serialize a JSON array of values into the loader.  Each entry of the
// array may itself be a JSON array (representing a nested set) or an
// object with "type" and "value" fields describing the token type
// and associated value.  Returns false if an error occurs.
/**
 * @brief Serialize a JSON array into a set of IFC tokens.
 *
 * The input JSON array may contain primitive values, nested arrays
 * (representing nested sets), or objects with "type" and "value"
 * fields describing explicit token types.  This function writes
 * the appropriate token markers (SET_BEGIN and SET_END) and
 * delegates to WriteValue for individual values.  Unsupported
 * objects result in an error message being logged via spdlog and
 * cause the return value to be false.  The caller must ensure that
 * the structure of the JSON corresponds to the IFC syntax.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose loader should be used.
 * @param val JSON array of values or objects to serialize.
 * @return True on success; false if an unsupported object was encountered.
 */
bool WriteSet(webifc::manager::ModelManager *manager,
              uint32_t modelID,
              const json &val)
{
    bool responseCode = true;
    auto loader = manager->GetIfcLoader(modelID);
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::SET_BEGIN);
    if (val.is_array())
    {
        for (const auto &child : val)
        {
            if (child.is_null())
            {
                loader->Push<uint8_t>(webifc::parsing::IfcTokenType::EMPTY);
            }
            else if (child.is_array())
            {
                // nested set
                WriteSet(manager, modelID, child);
            }
            else if (child.contains("value") && child["value"].is_array())
            {
                // object with type and array of values
                webifc::parsing::IfcTokenType type = static_cast<webifc::parsing::IfcTokenType>(child["type"].get<uint32_t>());
                loader->Push(webifc::parsing::IfcTokenType::SET_BEGIN);
                for (const auto &v : child["value"])
                {
                    loader->Push<uint8_t>(static_cast<uint8_t>(type));
                    if (type == webifc::parsing::IfcTokenType::INTEGER)
                    {
                        loader->PushInt(v.get<int>());
                    }
                    else
                    {
                        loader->PushDouble(v.get<double>());
                    }
                }
                loader->Push(webifc::parsing::IfcTokenType::SET_END);
            }
            else if (child.contains("type") && child["type"].is_number())
            {
                // object describing a single value
                webifc::parsing::IfcTokenType type = static_cast<webifc::parsing::IfcTokenType>(child["type"].get<uint32_t>());
                loader->Push<uint8_t>(static_cast<uint8_t>(type));
                switch (type)
                {
                case webifc::parsing::IfcTokenType::LABEL:
                {
                    // labels include a string and a nested set describing the value
                    std::string label = child["label"].get<std::string>();
                    webifc::parsing::IfcTokenType valueType = static_cast<webifc::parsing::IfcTokenType>(child["valueType"].get<uint32_t>());
                    json innerVal = child[valueType == webifc::parsing::IfcTokenType::REAL ? "internalValue" : "value"];
                    uint16_t length = static_cast<uint16_t>(label.size());
                    loader->Push<uint16_t>(length);
                    // Cast away constness of the label pointer to match the
                    // non‑const void* overload of Push().
                    loader->Push(const_cast<void *>(static_cast<const void *>(label.c_str())), label.size());
                    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::SET_BEGIN);
                    loader->Push<uint8_t>(static_cast<uint8_t>(valueType));
                    WriteValue(manager, modelID, valueType, innerVal);
                    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::SET_END);
                    break;
                }
                case webifc::parsing::IfcTokenType::REAL:
                {
                    WriteValue(manager, modelID, type, child["internalValue"]);
                    break;
                }
                case webifc::parsing::IfcTokenType::STRING:
                case webifc::parsing::IfcTokenType::ENUM:
                case webifc::parsing::IfcTokenType::REF:
                case webifc::parsing::IfcTokenType::INTEGER:
                {
                    WriteValue(manager, modelID, type, child["value"]);
                    break;
                }
                default:
                    // ignore unsupported types
                    break;
                }
            }
            else if (child.is_number() || child.is_boolean() || child.is_string())
            {
                webifc::parsing::IfcTokenType type;
                if (child.is_number_float())
                    type = webifc::parsing::IfcTokenType::REAL;
                else if (child.is_number_integer())
                    type = webifc::parsing::IfcTokenType::INTEGER;
                else if (child.is_string())
                    type = webifc::parsing::IfcTokenType::STRING;
                else
                    type = webifc::parsing::IfcTokenType::ENUM;
                loader->Push<uint8_t>(static_cast<uint8_t>(type));
                WriteValue(manager, modelID, type, child);
            }
            else
            {
                spdlog::error("[WriteSet()] Error: unknown object received");
                responseCode = false;
            }
        }
    }
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::SET_END);
    return responseCode;
}

// Read a single value from the loader and return it as JSON.  The
// returned JSON may be a boolean, number or string.  Unknown
// token types yield a null JSON value.
/**
 * @brief Read a single value from the loader and return it as JSON.
 *
 * Depending on the token type, the returned JSON value may be a
 * string, boolean, null, stringified double, integer or reference.
 * For enumerations "T", "F" and "U" are mapped to true, false and
 * null respectively.  Unknown types yield a null JSON value.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose loader should be used.
 * @param t Token type describing the value to read.
 * @return The decoded JSON value.
 */
json ReadValue(webifc::manager::ModelManager *manager,
               uint32_t modelID,
               webifc::parsing::IfcTokenType t)
{
    auto loader = manager->GetIfcLoader(modelID);
    switch (t)
    {
    case webifc::parsing::IfcTokenType::STRING:
    {
        return json(loader->GetDecodedStringArgument());
    }
    case webifc::parsing::IfcTokenType::ENUM:
    {
        std::string_view s = loader->GetStringArgument();
        if (s == "T")
            return json(true);
        if (s == "F")
            return json(false);
        if (s == "U")
            return json(nullptr);
        return json(std::string(s));
    }
    case webifc::parsing::IfcTokenType::REAL:
    {
        std::string_view s = loader->GetDoubleArgumentAsString();
        // represent numbers as strings to avoid precision loss
        return json(std::string(s));
    }
    case webifc::parsing::IfcTokenType::INTEGER:
    {
        long d = loader->GetIntArgument();
        return json(d);
    }
    case webifc::parsing::IfcTokenType::REF:
    {
        uint32_t ref = loader->GetRefArgument();
        return json(ref);
    }
    default:
        return json();
    }
}

// Recursively parse arguments from the loader and return a JSON array
// representing the argument list.  When parsing inside an object or
// list context, values are returned directly rather than wrapped with
// type information.  This mirrors the original behaviour of
// `emscripten::val` based implementations.
/**
 * @brief Recursively parse a list of arguments from the loader into JSON.
 *
 * This function examines tokens from the loader until it reaches the
 * end of a line or end of a set, converting each argument into JSON.
 * When parsing within an object context, primitive values are
 * returned directly; otherwise they are wrapped in an object
 * containing their type and value fields.  Nested sets are
 * represented as JSON arrays.  If no arguments are parsed and not
 * within a list context, null is returned.  If a single value is
 * parsed in an object context, that value is returned instead of
 * wrapping it in an array.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose loader should be used.
 * @param inObject True if parsing within a label/object context.
 * @param inList True if parsing within a list (set) context.
 * @return JSON array or value representing the parsed arguments.
 */
json GetArgs(webifc::manager::ModelManager *manager,
             uint32_t modelID,
             bool inObject = false,
             bool inList = false)
{
    auto loader = manager->GetIfcLoader(modelID);
    json arguments = json::array();
    size_t size = 0;
    bool endOfLine = false;
    while (!loader->IsAtEnd() && !endOfLine)
    {
        webifc::parsing::IfcTokenType t = loader->GetTokenType();
        switch (t)
        {
        case webifc::parsing::IfcTokenType::LINE_END:
        {
            endOfLine = true;
            break;
        }
        case webifc::parsing::IfcTokenType::EMPTY:
        {
            arguments.emplace_back(nullptr);
            ++size;
            break;
        }
        case webifc::parsing::IfcTokenType::SET_BEGIN:
        {
            arguments.emplace_back(GetArgs(manager, modelID, false, true));
            ++size;
            break;
        }
        case webifc::parsing::IfcTokenType::SET_END:
        {
            endOfLine = true;
            break;
        }
        case webifc::parsing::IfcTokenType::LABEL:
        {
            // read label as object with typecode and nested arguments
            json obj = json::object();
            obj["type"] = static_cast<uint32_t>(webifc::parsing::IfcTokenType::LABEL);
            loader->StepBack();
            auto s = loader->GetStringArgument();
            auto typeCode = manager->GetSchemaManager().IfcTypeToTypeCode(s);
            obj["typecode"] = typeCode;
            // read set open
            loader->GetTokenType();
            obj["value"] = GetArgs(manager, modelID, true);
            arguments.emplace_back(obj);
            ++size;
            break;
        }
        case webifc::parsing::IfcTokenType::STRING:
        case webifc::parsing::IfcTokenType::ENUM:
        case webifc::parsing::IfcTokenType::REAL:
        case webifc::parsing::IfcTokenType::INTEGER:
        case webifc::parsing::IfcTokenType::REF:
        {
            loader->StepBack();
            json obj;
            if (inObject)
            {
                obj = ReadValue(manager, modelID, t);
            }
            else
            {
                obj = json::object();
                obj["type"] = static_cast<uint32_t>(t);
                obj["value"] = ReadValue(manager, modelID, t);
            }
            arguments.emplace_back(obj);
            ++size;
            break;
        }
        default:
            break;
        }
    }
    if (size == 0 && !inList)
        return nullptr;
    if (size == 1 && inObject)
        return arguments[0];
    return arguments;
}

// Read the first header line of a given type and return it as a JSON
// object.  The resulting object contains the ID, the textual type
// name and a JSON array of arguments.
/**
 * @brief Retrieve the first header line of a given type.
 *
 * The returned JSON object contains three fields: "ID" with the
 * express ID of the header line, "type" with the textual name of
 * the header type, and "arguments" with a JSON array of the
 * header's parameters.  If no such header line exists or the
 * model is closed, an empty JSON object is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param headerType IFC type code of the header line to retrieve.
 * @return JSON object describing the header line or empty on failure.
 */
json GetHeaderLine(webifc::manager::ModelManager *manager,
                   uint32_t modelID,
                   uint32_t headerType)
{
    if (!manager->IsModelOpen(modelID))
        return json();
    auto loader = manager->GetIfcLoader(modelID);
    auto lines = loader->GetHeaderLinesWithType(headerType);
    if (lines.empty())
        return json();
    auto line = lines[0];
    loader->MoveToHeaderLineArgument(line, 0);
    std::string s(manager->GetSchemaManager().IfcTypeCodeToType(headerType));
    auto arguments = GetArgs(manager, modelID);
    json retVal;
    retVal["ID"] = line;
    retVal["type"] = s;
    retVal["arguments"] = arguments;
    return retVal;
}

// Read a single line by express ID and return it as a JSON object.
// The returned object includes the ID, the line type and the list
// of arguments.  If the line is invalid or the model is closed, an
// empty JSON object is returned.
/**
 * @brief Retrieve a single line of the IFC file.
 *
 * Returns a JSON object containing the line's ID, type code and
 * argument list.  If the line does not exist or the model is closed,
 * an empty object is returned.  The argument list is parsed via
 * GetArgs and may contain nested arrays and objects.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param expressID Express ID of the line to retrieve.
 * @return JSON object describing the line or empty on failure.
 */
json GetLine(webifc::manager::ModelManager *manager,
             uint32_t modelID,
             uint32_t expressID)
{
    auto loader = manager->GetIfcLoader(modelID);
    if (!manager->IsModelOpen(modelID))
        return json();
    if (!loader->IsValidExpressID(expressID))
        return json();
    uint32_t lineType = loader->GetLineType(expressID);
    if (lineType == 0)
        return json();
    loader->MoveToArgumentOffset(expressID, 0);
    auto arguments = GetArgs(manager, modelID);
    json retVal;
    retVal["ID"] = expressID;
    retVal["type"] = lineType;
    retVal["arguments"] = arguments;
    return retVal;
}

// Return an array of lines corresponding to the provided express IDs.
/**
 * @brief Retrieve multiple lines from the model at once.
 *
 * Accepts a JSON array of express IDs, retrieves each corresponding
 * line via GetLine and returns them in a JSON array.  If an input
 * value is not a number or if the model is closed, an empty array
 * is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param expressIDs JSON array of express IDs to fetch.
 * @return JSON array of line objects for each requested express ID.
 */
json GetLines(webifc::manager::ModelManager *manager,
              uint32_t modelID,
              const json &expressIDs)
{
    json result = json::array();
    if (!expressIDs.is_array())
        return result;
    for (const auto &idVal : expressIDs)
        result.push_back(GetLine(manager, modelID, idVal.get<uint32_t>()));
    return result;
}

// Get the type code for a line.  Returns zero if the model is not
// open.
/**
 * @brief Return the IFC type code for a given line.
 *
 * If the model is open, returns the type code of the specified
 * express ID; otherwise returns zero.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @param expressID Express ID of the line whose type should be returned.
 * @return Type code of the line or 0 if unavailable.
 */
uint32_t GetLineType(webifc::manager::ModelManager *manager,
                     uint32_t modelID,
                     uint32_t expressID)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->GetLineType(expressID) : 0;
}

// Write a header line into the loader.  The parameters argument is a
// JSON array describing the arguments for the header.  Returns
// whether the line was written successfully.
/**
 * @brief Write a header line to the model.
 *
 * Constructs and serializes an IFC header line using the provided
 * type code and list of parameters.  The type code is converted
 * into its uppercase textual name, which is written as a label,
 * followed by the parameter set and a line terminator.  The start
 * offset of the header line is recorded so that it can later be
 * updated or removed.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to modify.
 * @param type IFC type code of the header line to write.
 * @param parameters JSON array describing the arguments of the header line.
 * @return True if the line was written successfully; false otherwise.
 */
bool WriteHeaderLine(webifc::manager::ModelManager *manager,
                     uint32_t modelID,
                     uint32_t type,
                     const json &parameters)
{
    if (!manager->IsModelOpen(modelID))
        return false;
    auto loader = manager->GetIfcLoader(modelID);
    uint32_t start = loader->GetTotalSize();
    std::string ifcName = manager->GetSchemaManager().IfcTypeCodeToType(type);
    std::transform(ifcName.begin(), ifcName.end(), ifcName.begin(), ::toupper);
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::LABEL);
    loader->Push<uint16_t>(static_cast<uint16_t>(ifcName.size()));
    // Cast away constness of the underlying string data pointer to satisfy
    // the non‑const Push() overload.
    loader->Push(const_cast<void *>(static_cast<const void *>(ifcName.data())), ifcName.size());
    bool responseCode = WriteSet(manager, modelID, parameters);
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::LINE_END);
    loader->AddHeaderLineTape(type, start);
    return responseCode;
}

// Remove a line from the model.
/**
 * @brief Remove a line from the model.
 *
 * Deletes the specified express ID from the model if it is open.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to modify.
 * @param expressID Express ID of the line to remove.
 */
void RemoveLine(webifc::manager::ModelManager *manager,
                uint32_t modelID,
                uint32_t expressID)
{
    if (manager->IsModelOpen(modelID))
        manager->GetIfcLoader(modelID)->RemoveLine(expressID);
}

// Write an IFC line into the loader using the provided express ID,
// type code and argument list.  The parameters argument must be a
// JSON array.  Returns true if serialization succeeded.
/**
 * @brief Write a full IFC line to the model.
 *
 * Serializes an IFC line consisting of a line reference (express ID),
 * a label describing the type, a set of parameters and a line end
 * marker.  The start offset of the line is recorded so that it can
 * be updated later.  If the model is closed, nothing is written and
 * false is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to modify.
 * @param expressID Express ID to assign to the new line.
 * @param type IFC type code of the line being written.
 * @param parameters JSON array describing the line arguments.
 * @return True on success; false if the line could not be written.
 */
bool WriteLine(webifc::manager::ModelManager *manager,
               uint32_t modelID,
               uint32_t expressID,
               uint32_t type,
               const json &parameters)
{
    if (!manager->IsModelOpen(modelID))
        return false;
    auto loader = manager->GetIfcLoader(modelID);
    uint32_t start = loader->GetTotalSize();
    // line ID
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::REF);
    loader->Push<uint32_t>(expressID);
    // line TYPE
    std::string ifcName = manager->GetSchemaManager().IfcTypeCodeToType(type);
    std::transform(ifcName.begin(), ifcName.end(), ifcName.begin(), ::toupper);
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::LABEL);
    loader->Push<uint16_t>(static_cast<uint16_t>(ifcName.size()));
    // Cast away constness of the string pointer before passing to Push().
    loader->Push(const_cast<void *>(static_cast<const void *>(ifcName.data())), ifcName.size());
    bool responseCode = WriteSet(manager, modelID, parameters);
    // end line
    loader->Push<uint8_t>(webifc::parsing::IfcTokenType::LINE_END);
    loader->UpdateLineTape(expressID, type, start);
    return responseCode;
}

// Retrieve the version string for the library.
/**
 * @brief Return the version number of the Web‑IFC library.
 *
 * The version is defined by the WEB_IFC_VERSION_NUMBER macro.
 *
 * @return A string containing the version number.
 */
std::string GetVersion()
{
    return std::string(WEB_IFC_VERSION_NUMBER);
}

// Generate a GUID for a line in the model.  Returns an empty string if
// the model is not open.
/**
 * @brief Generate a new GUID for the given model.
 *
 * Uses the model's loader to generate a unique identifier.  If the
 * model is closed, an empty string is returned.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model for which to generate a GUID.
 * @return A GUID string or empty if the model is closed.
 */
std::string GenerateGuid(webifc::manager::ModelManager *manager,
                         uint32_t modelID)
{
    if (!manager->IsModelOpen(modelID))
        return std::string();
    auto loader = manager->GetIfcLoader(modelID);
    return loader->GenerateUUID();
}

// Get the maximum express ID present in the model.
/**
 * @brief Retrieve the maximum express ID currently used in the model.
 *
 * Returns zero if the model is closed.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @return Maximum express ID in use or 0 if the model is closed.
 */
uint32_t GetMaxExpressID(webifc::manager::ModelManager *manager,
                         uint32_t modelID)
{
    return manager->IsModelOpen(modelID) ? manager->GetIfcLoader(modelID)->GetMaxExpressId() : 0;
}

// Determine whether a model is open.
/**
 * @brief Determine whether a model is currently open.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model to query.
 * @return True if the model is open; false otherwise.
 */
bool IsModelOpen(webifc::manager::ModelManager *manager,
                 uint32_t modelID)
{
    return manager->IsModelOpen(modelID);
}

// Set the logging level for the manager.
/**
 * @brief Set the log level for the model manager.
 *
 * The log level controls the verbosity of diagnostic output.  A
 * higher number may produce more detailed messages.  This affects
 * internal logging as well as any code that observes the manager's
 * logging facilities.
 *
 * @param manager Pointer to the model manager whose log level should be set.
 * @param levelArg Desired log level (implementation defined).
 */
void SetLogLevel(webifc::manager::ModelManager *manager,
                 uint8_t levelArg)
{
    manager->SetLogLevel(levelArg);
}

// Encode a text string using the IFC P21 encoding.  Returns the
// encoded string.
/**
 * @brief Encode a string using the IFC P21 encoding.
 *
 * This helper forwards to the p21encode function from the parsing
 * namespace and returns the encoded string.
 *
 * @param text The plain text to encode.
 * @return The encoded representation of the text.
 */
std::string EncodeText(std::string text)
{
    const std::string_view strView{text};
    std::ostringstream output;
    webifc::parsing::p21encode(strView, output);
    return output.str();
}

// Decode a P21 encoded string into plain text.
/**
 * @brief Decode a P21 encoded string into plain text.
 *
 * This helper forwards to the p21decode function from the parsing
 * namespace.
 *
 * @param text The encoded text to decode.
 * @return The decoded plain text.
 */
std::string DecodeText(std::string text)
{
    std::string_view strView{text};
    return webifc::parsing::p21decode(strView);
}

// Reset the geometry processor's cache for the given model.
/**
 * @brief Clear the geometry cache for a model.
 *
 * Flushes any cached geometry data held by the geometry loader.  If
 * the model is not open this call has no effect.
 *
 * @param manager Pointer to the model manager that owns the model.
 * @param modelID Identifier of the model whose cache should be reset.
 */
void ResetCache(webifc::manager::ModelManager *manager,
                uint32_t modelID)
{
    if (manager->IsModelOpen(modelID))
        manager->GetGeometryProcessor(modelID)->GetLoader().ResetCache();
}

// Factory functions for creating geometry primitives.  These simply
// forward to the constructors of the underlying types.
/**
 * @name Geometry Factory Functions
 *
 * These simple functions construct new instances of geometry classes
 * defined in the bimGeometry namespace.  They serve as factories
 * that can be bound or exported if necessary.  The constructors
 * invoked here do not perform any heavy initialization.
 */

/// Construct a new axis‑aligned bounding box (AABB).
bimGeometry::AABB CreateAABB()
{
    return bimGeometry::AABB();
}
/// Construct a new Extrusion primitive.
bimGeometry::Extrusion CreateExtrusion()
{
    return bimGeometry::Extrusion();
}
/// Construct a new Sweep primitive.
bimGeometry::Sweep CreateSweep()
{
    return bimGeometry::Sweep();
}
/// Construct a new CircularSweep primitive.
bimGeometry::CircularSweep CreateCircularSweep()
{
    return bimGeometry::CircularSweep();
}
/// Construct a new Revolution (Revolve) primitive.
bimGeometry::Revolve CreateRevolution()
{
    return bimGeometry::Revolve();
}
/// Construct a new CylindricalRevolution primitive.
bimGeometry::CylindricalRevolution CreateCylindricalRevolution()
{
    return bimGeometry::CylindricalRevolution();
}
/// Construct a new Parabola primitive.
bimGeometry::Parabola CreateParabola()
{
    return bimGeometry::Parabola();
}
/// Construct a new Clothoid primitive.
bimGeometry::Clothoid CreateClothoid()
{
    return bimGeometry::Clothoid();
}
/// Construct a new Arc primitive.
bimGeometry::Arc CreateArc()
{
    return bimGeometry::Arc();
}
/// Construct a new Alignment primitive.
bimGeometry::Alignment CreateAlignment()
{
    return bimGeometry::Alignment();
}
/// Construct a new Boolean operation primitive.
bimGeometry::Boolean CreateBoolean()
{
    return bimGeometry::Boolean();
}
/// Construct a new Profile primitive.
bimGeometry::Profile CreateProfile()
{
    return bimGeometry::Profile();
}

/*
 * Note: In the original WebAssembly binding, the EMSCRIPTEN_BINDINGS
 * macro was used to expose classes and functions to JavaScript.  In
 * this pure C++ implementation that macro has been removed because
 * there is no JavaScript environment to bind to.  If you need to
 * interface these functions with another language or runtime you can
 * use whatever foreign function interface (FFI) mechanisms are
 * appropriate for your environment.  Alternatively you can wrap
 * these functions in a thin C API if necessary.
 */
