#version 460 core
#extension GL_GOOGLE_include_directive : require

// 
#include "./include/driver.glsl"
#include "./include/constants.glsl"
#include "./include/common.glsl"
#include "./include/framebuffer.glsl"

//
layout (location = 0) in vec2 vcoord;
layout (location = 0) out vec4 fragColor;

// show ray tracing results
void main(){
    vec3 color = texture(imageBuffers[0], vcoord).xyz;
    fragColor = vec4(color, 1.f);
};
