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

#endif
