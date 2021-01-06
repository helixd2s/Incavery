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

vec4 divW(in vec4 pos) {
    return pos/pos.w;
};

// inverse mat3x4
mat3x4 inverse(in mat3x4 a) {
    mat4x4 c = transpose(inverse(transpose(mat4x4(a[0],a[1],a[2],vec4(0.f.xxx,1.f)))));
    return mat3x4(c[0],c[1],c[2]);
};


#endif
