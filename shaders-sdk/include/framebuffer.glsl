#ifndef FRAMEBUFFER_GLSL
#define FRAMEBUFFER_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"
#include "./instanceLevel.glsl"
#include "./material.glsl"

#ifndef FRAMEBUFFER_MAP
#define FRAMEBUFFER_MAP 0
#endif

layout (binding = 0, set = FRAMEBUFFER_MAP, rgba32f) uniform image2D imageBuffers[];

#ifdef FRAGMENT
layout (location = 0) out vec4 fBarycentrics;
layout (location = 1) out vec4 fIndices;
#endif

// non-RTX version of intersection (only first pass)
IntersectionInfo rasterization(in RayData rays, in float maxT) {
    uvec4 indices = floatBitsToUint(imageLoad(imageBuffers[1], ivec2(rays.launchId)));

    // 
    IntersectionInfo result;
    result.barycentric = imageLoad(imageBuffers[0], ivec2(rays.launchId)).xyz;
    result.instanceId = indices.x;
    result.geometryId = indices.y;
    result.primitiveId = indices.z;
    result.hitT = maxT;

    // compute hitT from rasterization
    if (any(greaterThan(result.barycentric, 0.f.xxx))) {
        GeometryInfo geometryInfo = readGeometryInfo(result.instanceId, result.geometryId);
        mat3x4 vertices = readVertices(geometryInfo.vertex, result.primitiveId);
        vec4 origin = vertices * result.barycentric;
        result.hitT = distance(rays.origin.xyz, origin.xyz);
    };

    return result;
};

#endif
