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
    uint32_t customIndex24_mask8;
    uint32_t sbtOffset24_flags8;
    uint64_t reference;
};

//
layout (binding = 0, set = INSTANCE_LEVEL_MAP, scalar) buffer InstanceBuffer { InstanceInfo instances[]; };

// 
GeometryInfo readGeometryInfo(in uint instanceId, in uint geometryId) 
{
    uint customIndex = bitfieldExtract(instances[instanceId].customIndex24_mask8, 0, 24);
    return registry[customIndex].geometries[geometryId];
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
