#ifndef INSTANCE_LEVEL_GLSL
#define INSTANCE_LEVEL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"

#ifndef INSTANCE_LEVEL_MAP
#define INSTANCE_LEVEL_MAP 2
#endif

// 
struct InstanceInfo 
{
    mat3x4 transform;
    uint32_t customIndex24_mask8;
    uint32_t sbtOffset24_flags8;
    uint64_t reference;
};

//
layout (binding = 0, set = INSTANCE_LEVEL_MAP, scalar) buffer InstanceBuffer { InstanceInfo instances[]; };

// 
//#define bindingInfo bindings[bindingId]

// 
struct Attributes 
{
    int32_t texcoords;
    int32_t normals;
    int32_t tangents;
    int32_t colors;
};

// 
struct VertexInfo 
{
    uint32_t bufferId;
    uint32_t stride;
};

// 
struct IndexInfo 
{
    int32_t first;
    uint32_t max;
    uint32_t bufferId;
    uint32_t type; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
};

//
struct PrimitiveInfo 
{
    uint32_t offset;
    uint32_t count;
    uint32_t materials;
};

// 
struct GeometryInfo 
{
    mat3x4 transform;

    uint32_t flags;

    VertexInfo vertex;
    IndexInfo index;
    PrimitiveInfo primitive;

    Attributes attributes;
};

// 
layout (binding = 2, set = INSTANCE_LEVEL_MAP, scalar) buffer GeometryBuffer { GeometryInfo geometries[]; } registry[];

//
struct AttributeMap 
{
    mat3x4 texcoords;
    mat3x4 normals;
    mat3x4 tangents;
    mat3x4 colors;
};

//
struct AttributeInterpolated 
{
    vec4 texcoords;
    vec4 normals;
    vec4 tangents;
    vec4 colors;
};

// fixed 06.01.2021
uint readIndex(inout IndexInfo indexInfo, in uint verticeId) 
{
    if (indexInfo.type == 1u) { return (indexInfo.first + readUint32(indexInfo.bufferId, verticeId*4u)); };
    if (indexInfo.type == 2u) { return (indexInfo.first + readUint16(indexInfo.bufferId, verticeId*2u)); };
    if (indexInfo.type == 3u) { return (indexInfo.first + readUint8(indexInfo.bufferId, verticeId)); };
    return (indexInfo.first + verticeId);
};

// fixed 06.01.2021
uvec3 readIndices(inout IndexInfo indexInfo, in uint primitiveId) 
{
    return uvec3(readIndex(indexInfo, primitiveId*3u), readIndex(indexInfo, primitiveId*3u+1u), readIndex(indexInfo, primitiveId*3u+2u));
};

// TODO: support for "primitiveOffset"
vec4 readVertex(inout VertexInfo vertexInfo, in uint index) 
{
    uint offset = vertexInfo.stride * index;
    return vec4(readFloat4(vertexInfo.bufferId, offset).xyz, 1.f);
};

//
mat3x4 readVertices(inout VertexInfo vertexInfo, in uvec3 indices) 
{
    return mat3x4(
        readVertex(vertexInfo, indices.x),
        readVertex(vertexInfo, indices.y),
        readVertex(vertexInfo, indices.z)
    );
};

// read material ID from buffer by primitiveId
uint readMaterial(inout PrimitiveInfo primitiveInfo, in uint primitiveId)
{
    uint bindingId = primitiveInfo.materials;
    uint offset = bindingInfo.stride * primitiveId + bindingInfo.offset;
    return readUint32(bindingInfo.bufferId, offset);
};

// 
GeometryInfo readGeometryInfo(in uint instanceId, in uint geometryId) 
{
    uint customIndex = bitfieldExtract(instances[instanceId].customIndex24_mask8, 0, 24);
    return registry[customIndex].geometries[geometryId];
};

// TODO: use normals/texcoords/colors indices
AttributeMap readAttributes(inout Attributes attributes, in uvec3 indices) 
{
    AttributeMap map;
    map.texcoords = attributes.texcoords >= 0 ? readAttribute(attributes.texcoords, indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.normals   = attributes.normals   >= 0 ? readAttribute(attributes.normals  , indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.tangents  = attributes.tangents  >= 0 ? readAttribute(attributes.tangents , indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.colors    = attributes.colors    >= 0 ? readAttribute(attributes.colors   , indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    return map;
};

//
AttributeInterpolated interpolateAttributes(inout AttributeMap map, in vec3 barycentric) 
{
    AttributeInterpolated interpolated;
    interpolated.texcoords = map.texcoords * barycentric;
    interpolated.normals = map.normals * barycentric;
    interpolated.tangents = map.tangents * barycentric;
    interpolated.colors = map.colors * barycentric;
    return interpolated;
};

#endif
