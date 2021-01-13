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
    uint32_t format;
    uint32_t stride;
    RawData ptr;
};

// 
struct IndexInfo 
{
    int32_t first;
    uint32_t max;
    uint32_t type; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
    uint32_t reserved;
    RawData ptr;
};

//
struct PrimitiveInfo 
{
    uint32_t offset;
    uint32_t count;
    uint32_t materials;
    uint32_t reserved;
};

// 
struct GeometryInfo 
{
    mat3x4 transform;

    uint32_t flags;
    uint32_t reserved;

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
    if (indexInfo.type == 1u) { return (indexInfo.first + readUint32(indexInfo.ptr, verticeId*4u)); };
    if (indexInfo.type == 2u) { return (indexInfo.first + readUint16(indexInfo.ptr, verticeId*2u)); };
    if (indexInfo.type == 3u) { return (indexInfo.first + readUint8(indexInfo.ptr, verticeId)); };
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
    return vec4(readFloat4(vertexInfo.ptr, offset).xyz, 1.f);
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
    uint offset = bindingInfo.stride * primitiveId;
    return readUint32(bindingInfo.ptr, offset);
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

//
AttributeMap surroundNormals(inout AttributeMap map, in mat3x4 vertices)
{
    vec3 geometryNormal = normalize(cross(vertices[1].xyz - vertices[0].xyz, vertices[2].xyz - vertices[0].xyz));
    for (int i=0;i<3;i++) {
        if (length(map.normals[i].xyz) < 0.0001f) { map.normals[i].xyz = geometryNormal; };
    };
    return map;
};

//
AttributeInterpolated surroundNormal(inout AttributeInterpolated attributes, in mat3x4 vertices)
{
    vec3 geometryNormal = normalize(cross(vertices[1].xyz - vertices[0].xyz, vertices[2].xyz - vertices[0].xyz));
    if (length(attributes.normals.xyz) < 0.0001f) {
        attributes.normals.xyz = geometryNormal;
    };
    return attributes;
};


//
AttributeInterpolated transformNormal(inout AttributeInterpolated attributes, in uint instanceId, in uint geometryId)
{
    GeometryInfo geometryInfo = readGeometryInfo(instanceId, geometryId);
    InstanceInfo instanceInfo = instances[instanceId];
    attributes.normals = vec4(vec4(vec4(attributes.normals.xyz, 0.f) * geometryInfo.transform, 0.f) * instanceInfo.transform, 0.f);
    return attributes;
};

//
AttributeMap transformNormals(inout AttributeMap map, in uint instanceId, in uint geometryId)
{
    GeometryInfo geometryInfo = readGeometryInfo(instanceId, geometryId);
    InstanceInfo instanceInfo = instances[instanceId];
    for (int i=0;i<3;i++) {
        //map.normals[i] = vec4(vec4(vec4(map.normals[i].xyz, 0.f) * geometryInfo.transform, 0.f) * instanceInfo.transform, 0.f);
        map.normals[i] = vec4(vec4(vec4(map.normals[i].xyz, 0.f) * inverse(geometryInfo.transform), 0.f) * inverse(instanceInfo.transform), 0.f);
    };
    return map;
};

//
mat3x4 transformVertices(inout mat3x4 vertices, in uint instanceId, in uint geometryId) 
{
    GeometryInfo geometryInfo = readGeometryInfo(instanceId, geometryId);
    InstanceInfo instanceInfo = instances[instanceId];
    for (int i=0;i<3;i++) {
        vertices[i] = vec4(vec4(vec4(vertices[i].xyz, 1.f) * geometryInfo.transform, 1.f) * instanceInfo.transform, 1.f);
    };
    return vertices;
};


#endif
