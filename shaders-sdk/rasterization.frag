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
#include "./include/external.glsl"
//#include "./include/rayTracing.glsl"

//
#ifdef OPAQUE
layout ( early_fragment_tests ) in;
#endif

// 
layout (location = 0) in vec4 transformed;
layout (location = 1) in vec4 original;
layout (location = 2) in vec4 barycentric;
layout (location = 3) in vec4 normals;
layout (location = 4) flat in uint primitiveId;
layout (location = 5) flat in uint vertexIndex;
//layout (location = 3) flat in uvec4 indices;

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

    // 
    uvec3 indices = readIndices(geometryInfo.index, primitiveId);
    AttributeMap attributeMap = readAttributes(geometryInfo.attributes, indices);
    AttributeInterpolated attributes = interpolateAttributes(attributeMap, barycentric.xyz);
    MaterialInfo material = handleMaterial(geometryInfo.primitive.materials, attributes);

    // 
    if (material.baseColorFactor.a < 0.0001f) {
        gl_FragDepth = 1.f;

        // required extension
        //discard;
    } else {
        // finalize fragment results
        gl_FragDepth = gl_FragCoord.z;
        fBarycentrics = barycentric;
        fIndices = uvec4(pushed.instanceId, pushed.geometryId, primitiveId, 0u);//indices;
        fSRAA = vec4(normals.xyz, gl_FragCoord.z);
    };
};
