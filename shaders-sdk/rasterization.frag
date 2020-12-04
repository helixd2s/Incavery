#version 460 core
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing          : require
#extension GL_EXT_ray_query            : require
#extension GL_ARB_post_depth_coverage  : require

// 
#define FRAGMENT

// 
#include "./include/driver.glsl"
#include "./include/constants.glsl"
#include "./include/common.glsl"
#include "./include/framebuffer.glsl"
#include "./include/geometryRegistry.glsl"
#include "./include/instanceLevel.glsl"
#include "./include/material.glsl"
//#include "./include/rayTracing.glsl"

//
#ifdef OPAQUE
layout ( early_fragment_tests ) in;
#endif

// 
layout (location = 0) in vec4 transformed;
layout (location = 1) in vec4 original;
layout (location = 2) in vec4 barycentric;
layout (location = 3) flat in uvec4 indices;

// 
layout(push_constant) uniform pushConstants {
    uint instanceId;
    uint geometryId;
    uint reserved0;
    uint reserved1;
} pushed;

// 
void main() 
{
    GeometryInfo geometryInfo = readGeometryInfo(pushed.instanceId, pushed.geometryId);
    InstanceInfo instanceInfo = instances[pushed.instanceId];

    // TODO: filter opaque
    // 

    // finalize fragment results
    fBarycentrics = barycentric;
    fIndices = indices;
};
