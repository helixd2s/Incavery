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

layout (binding = 0, set = FRAMEBUFFER_MAP) uniform sampler2D imageBuffers[];

#ifdef FRAGMENT
layout (location = 0) out vec4 fBarycentrics;
layout (location = 1) out vec4 fIndices;
layout (location = 2) out vec4 fSRAA;
#endif

// non-RTX version of intersection (only first pass)
IntersectionInfo rasterization(in RayData rays, in float maxT) {
    uvec4 indices = floatBitsToUint(texelFetch(imageBuffers[1], ivec2(rays.launchId), 0));

    // 
    IntersectionInfo result;
    result.barycentric = texelFetch(imageBuffers[0], ivec2(rays.launchId), 0).xyz;
    result.instanceId = indices.x;
    result.geometryId = indices.y;
    result.primitiveId = indices.z;
    result.hitT = maxT;

    // compute hitT from rasterization
    if (any(greaterThan(result.barycentric, 0.f.xxx))) {
        GeometryInfo geometryInfo = readGeometryInfo(result.instanceId, result.geometryId);
        mat3x4 vertices = readBindings3x4(geometryInfo.vertex, readIndices(geometryInfo.index, result.primitiveId));
        vec4 origin = vertices * result.barycentric;
        result.hitT = distance(rays.origin.xyz, origin.xyz);
    };

    return result;
};

#endif
