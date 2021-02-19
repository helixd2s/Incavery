#ifndef DRAW_INSTANCE_LEVEL_GLSL
#define DRAW_INSTANCE_LEVEL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"
#include "./geometryLevel.glsl"

// 
#ifndef DRAW_INSTANCE_LEVEL_MAP
#define DRAW_INSTANCE_LEVEL_MAP 3
#endif

//
struct DrawIndirect 
{
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

// 
layout(buffer_reference, scalar) buffer DrawIndirectBuffer {
    DrawIndirect geometries[];
};

// 
struct DrawInstanceInfo 
{
    mat3x4 transform;

    uint32_t programId;
    uint32_t geometryLevelId;
    uint32_t geometryLevelCount;
    uint32_t reserved;

    GeometryLevel geometryLevelReference;
    uint64_t geometryLevelIndirectReference;
    DrawIndirectBuffer drawIndirectReference;
    //uint64_t reserved0;
};

//
layout (binding = 0, set = DRAW_INSTANCE_LEVEL_MAP, scalar) buffer DrawInstanceBuffer { DrawInstanceInfo drawInstances[]; };
//layout (binding = 1, set = DRAW_INSTANCE_LEVEL_MAP, scalar) buffer DrawIndirectBuffer { DrawIndirect geometries[]; } drawInstancesIndirect[];

// 
GeometryInfo readGeometryInfo(inout DrawInstanceInfo instance, in uint geometryId) 
{
    return instance.geometryLevelReference.geometries[geometryId];
};

// 
GeometryInfo readGeometryInfoFromDrawInstance(in uint instanceId, in uint geometryId) 
{
    return readGeometryInfo(drawInstances[instanceId], geometryId);
};

//
AttributeInterpolated transformNormalFromDrawInstance(inout AttributeInterpolated attributes, in uint instanceId, in uint geometryId)
{
    DrawInstanceInfo instanceInfo = drawInstances[instanceId];
    GeometryInfo geometryInfo = readGeometryInfo(instanceInfo, geometryId);
    attributes.normals = vec4(vec4(vec4(attributes.normals.xyz, 0.f) * geometryInfo.transform, 0.f) * instanceInfo.transform, 0.f);
    return attributes;
};

//
AttributeMap transformNormalsFromDrawInstance(inout AttributeMap map, in uint instanceId, in uint geometryId)
{
    DrawInstanceInfo instanceInfo = drawInstances[instanceId];
    GeometryInfo geometryInfo = readGeometryInfo(instanceInfo, geometryId);
    for (int i=0;i<3;i++) {
        //map.normals[i] = vec4(vec4(vec4(map.normals[i].xyz, 0.f) * geometryInfo.transform, 0.f) * instanceInfo.transform, 0.f);
        map.normals[i] = vec4(vec4(vec4(map.normals[i].xyz, 0.f) * inverse(geometryInfo.transform), 0.f) * inverse(instanceInfo.transform), 0.f);
    };
    return map;
};

//
mat3x4 transformVerticesFromDrawInstance(inout mat3x4 vertices, in uint instanceId, in uint geometryId) 
{
    DrawInstanceInfo instanceInfo = drawInstances[instanceId];
    GeometryInfo geometryInfo = readGeometryInfo(instanceInfo, geometryId);
    for (int i=0;i<3;i++) {
        vertices[i] = vec4(vec4(vec4(vertices[i].xyz, 1.f) * geometryInfo.transform, 1.f) * instanceInfo.transform, 1.f);
    };
    return vertices;
};


#endif
