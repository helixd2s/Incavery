#version 460 core
#extension GL_GOOGLE_include_directive : require

// 
#include "./include/driver.glsl"
#include "./include/constants.glsl"
#include "./include/common.glsl"
#include "./include/framebuffer.glsl"
#include "./include/geometryRegistry.glsl"
#include "./include/instanceLevel.glsl"
#include "./include/drawInstanceLevel.glsl"
#include "./include/material.glsl"
#include "./include/external.glsl"

// 
const float3 bary[3] = { float3(1.f,0.f,0.f), float3(0.f,1.f,0.f), float3(0.f,0.f,1.f) };

//
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

//
//layout (location = 0) in vec4 position[3];
//layout (location = 1) in flat uint indices[3];
layout (location = 0) in flat uvec4 passed[3];

// 
layout (location = 0) out vec4 transformed;
layout (location = 1) out vec4 original;
layout (location = 2) out vec4 barycentric;
layout (location = 3) out vec4 normals;
layout (location = 4) flat out uvec4 parameters;

#define primitiveId parameters.x
#define vertexIndex parameters.y
#define drawIndex parameters.z

#define gl_DrawID passed[0].x

//gl_DrawID

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
    uint geometryId = pushed.geometryId + gl_DrawID; // fate with `gl_DrawID`
    GeometryInfo geometryInfo = readGeometryInfoFromDrawInstance(pushed.instanceId, geometryId);
    uvec3 indices = readIndices(geometryInfo.index, gl_PrimitiveIDIn); // please, always use correct "gl_PrimitiveIDIn"
    mat3x4 objectspace = readBindings3x4(bindings[geometryInfo.vertex], indices); // BROKEN!
    //mat3x4(
    //    vec4(1.f, -1.f, 1.f, 1.f),
    //    vec4(-1.f, -1.f, 1.f, 1.f),
    //    vec4(0.f,  1.f, 1.f, 1.f)
    //);//

    // 
    transformVerticesFromDrawInstance(objectspace, pushed.instanceId, geometryId);
    normals = vec4(normalize(cross(objectspace[1].xyz-objectspace[0].xyz, objectspace[2].xyz-objectspace[0].xyz)), 1.f);

    // finalize results
    parameters = uvec4(0u,0u,0u,0u);
    primitiveId = gl_PrimitiveIDIn;
    drawIndex = gl_DrawID;

    // 
    for (int i=0;i<3;i++) 
    {
        transformed = objectspace[i];
        barycentric = vec4(bary[i], 1.f);
        vertexIndex = indices[i];
        drawIndex = gl_DrawID;

        // TODO: perspective projection
        gl_Position = vec4(transformed * constants.lookAt, 1.f) * constants.perspective;
        EmitVertex();
    };
    EndPrimitive();

};
