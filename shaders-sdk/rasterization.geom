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
#include "./include/external.glsl"

// 
const float3 bary[3] = { float3(1.f,0.f,0.f), float3(0.f,1.f,0.f), float3(0.f,0.f,1.f) };

//
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

//
layout (location = 0) in vec4 position[3];
layout (location = 1) in flat uint indices[3];

// 
layout (location = 0) out vec4 transformed;
layout (location = 1) out vec4 original;
layout (location = 2) out vec4 barycentric;
layout (location = 3) out vec4 normals;
layout (location = 4) flat out uint primitiveId;
layout (location = 5) flat out uint vertexIndex;

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
    // 
    mat3x4 objectspace = mat3x4(position[0], position[1], position[2]);
    transformVertices(objectspace, pushed.instanceId, pushed.geometryId);
    normals = vec4(normalize(cross(objectspace[1].xyz-objectspace[0].xyz, objectspace[2].xyz-objectspace[0].xyz)), 1.f);

    // finalize results
    primitiveId = gl_PrimitiveIDIn;
    for (int i=0;i<3;i++) 
    {
        original = position[i];
        transformed = objectspace[i];
        barycentric = vec4(bary[i], 1.f);
        vertexIndex = indices[i];

        // TODO: perspective projection
        gl_Position = vec4(transformed * constants.lookAt, 1.f) * constants.perspective;
        EmitVertex();
    };
    EndPrimitive();

};
