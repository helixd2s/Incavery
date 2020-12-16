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
layout (location = 0) in uvec4 vertex; // it may to be float16
layout (location = 0) out vec4 position;
layout (location = 1) out flat uint indices;

// 
void main() 
{
    indices = gl_VertexIndex;
    position = vec4(uintBitsToFloat(vertex.xyz), 1.f); // TODO: fp16 support
    gl_Position = position;
};
