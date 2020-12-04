#ifndef RAY_TRACING_GLSL
#define RAY_TRACING_GLSL

struct RayData
{
    vec4 origin;
    vec4 direction;
    
};

struct IntersectionInfo 
{
    vec3 barycentric; float hitT;
    uint instanceId, geometryId, primitiveId, reserved0;
};

#endif
