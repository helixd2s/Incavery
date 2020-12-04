#version 460 core
#extension GL_GOOGLE_include_directive : require

// 
#include "./include/driver.glsl"
#include "./include/constants.glsl"
#include "./include/common.glsl"
#include "./include/framebuffer.glsl"
#include "./include/geometryRegistry.glsl"
#include "./include/instanceLevel.glsl"
#include "./include/material.glsl"

// 
const float3 bary[3] = { float3(1.f,0.f,0.f), float3(0.f,1.f,0.f), float3(0.f,0.f,1.f) };

//
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

//
layout (location = 0) in vec4 position[3];

// 
layout (location = 0) out vec4 transformed;
layout (location = 1) out vec4 original;
layout (location = 2) out vec4 barycentric;
layout (location = 3) flat out uvec4 indices;

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

    for (int i=0;i<3;i++) 
    {
        
        
    };

    // finalize results
    indices = uvec4(pushed.instanceId, pushed.geometryId, gl_PrimitiveIDIn, 0u);
    for (int i=0;i<3;i++) 
    {
        original = position[i];
        transformed = vec4(vec4(position[i] * geometryInfo.transform, 1.f) * instanceInfo.transform, 1.f);
        barycentric = vec4(bary[i], 1.f);
        

        // TODO: perspective projection
        gl_Position = transformed; // * perspective;
        EmitVertex();
    };
    EndPrimitive();

};
