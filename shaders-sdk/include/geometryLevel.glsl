#ifndef GEOMETRY_LEVEL_GLSL
#define GEOMETRY_LEVEL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"

#ifndef INSTANCE_LEVEL_MAP
#define INSTANCE_LEVEL_MAP 2
#endif

// contain ONLY links into bindings buffer
struct Attributes 
{
    int32_t texcoords;
    int32_t normals;
    int32_t tangents;
    int32_t colors;
};

// 
struct IndexInfo 
{
    int32_t first; 
    uint32_t max;
    uvec3 reserved;
    uint32_t type; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
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
    uint32_t type; // zero is triangles

    uint32_t aabbs; // needed for custom geometry types
    uint32_t vertex;

    IndexInfo index; // suitable for triangles, but can be used by custom geometry if defined usage manually
    PrimitiveInfo primitive;

    Attributes attributes; // REQUIRED for SOME triangles, so we dedicated into that block
};

// 
//layout (binding = 2, set = INSTANCE_LEVEL_MAP, scalar) buffer GeometryBuffer { GeometryInfo geometries[]; } registry[];

layout(buffer_reference, scalar) buffer GeometryLevel {
    GeometryInfo geometries[];
};

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

// TODO: use normals/texcoords/colors indices
AttributeMap readAttributes3x4(inout Attributes attributes, in uvec3 indices) 
{
    AttributeMap map;
    map.texcoords = attributes.texcoords >= 0 ? readBindings3x4(bindings[attributes.texcoords], indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.normals   = attributes.normals   >= 0 ? readBindings3x4(bindings[attributes.normals  ], indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.tangents  = attributes.tangents  >= 0 ? readBindings3x4(bindings[attributes.tangents ], indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
    map.colors    = attributes.colors    >= 0 ? readBindings3x4(bindings[attributes.colors   ], indices) : mat3x4(vec4(0.f.xxxx), vec4(0.f.xxxx), vec4(0.f.xxxx));
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




#endif
