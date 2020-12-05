#ifndef COMMON_GLSL
#define COMMON_GLSL

#include "./driver.glsl"
#include "./constants.glsl"

struct RayData
{
    vec4 origin;
    vec4 direction;
    u16vec2 launchId;
    uvec3 reserved0;
};

struct IntersectionInfo 
{
    vec3 barycentric; float hitT;
    uint instanceId, geometryId, primitiveId, reserved0;
};

//
layout(binding = 0, set = 3) uniform Constants {
    mat4x4 perspective;
    mat4x4 perspectiveInverse;
    mat3x4 lookAt;
    mat3x4 lookAtInverse;
} constants;

vec4 divW(in vec4 pos) {
    return pos/pos.w;
};

#endif
