#ifndef INSTANCE_LEVEL_GLSL
#define INSTANCE_LEVEL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"
#include "./geometryLevel.glsl"

#ifndef INSTANCE_LEVEL_MAP
#define INSTANCE_LEVEL_MAP 2
#endif

// 
struct InstanceInfo 
{
    mat3x4 transform;

    uint8_t mask;
    uint8_t flags;
    u8vec2 todo;
    uint32_t sbtOffsetId;

    uint32_t geometrylevelId;
    uint32_t geometryLevelCount;

    GeometryLevel geometryLevelReference;
    accelerationStructureEXT accelerationReference;
};

//
layout (binding = 0, set = INSTANCE_LEVEL_MAP, scalar) buffer InstanceBuffer { InstanceInfo instances[]; };

// 
GeometryInfo readGeometryInfo(inout InstanceInfo instance, in uint geometryId) 
{
    return instance.geometryLevelReference.geometries[geometryId];
};

// 
GeometryInfo readGeometryInfo(in uint instanceId, in uint geometryId) 
{
    return readGeometryInfo(instances[instanceId], geometryId);
};

//
AttributeInterpolated transformNormal(inout AttributeInterpolated attributes, in uint instanceId, in uint geometryId)
{
    InstanceInfo instanceInfo = instances[instanceId];
    GeometryInfo geometryInfo = readGeometryInfo(instanceInfo, geometryId);
    attributes.normals = vec4(vec4(vec4(attributes.normals.xyz, 0.f) * geometryInfo.transform, 0.f) * instanceInfo.transform, 0.f);
    return attributes;
};

//
AttributeMap transformNormals(inout AttributeMap map, in uint instanceId, in uint geometryId)
{
    InstanceInfo instanceInfo = instances[instanceId];
    GeometryInfo geometryInfo = readGeometryInfo(instanceInfo, geometryId);
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
