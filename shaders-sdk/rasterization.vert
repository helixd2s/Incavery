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
layout (location = 0) in vec4 vertex; 
layout (location = 0) out vec4 position;

// 
void main() 
{
    position = vec4(vertex.xyz, 1.f);
};