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

// 
const float nscale = 1.f;
const float dscale = 1.f;

// 
float bilateral(in vec4 cnd, in vec4 tnd) {
    return exp(-nscale * max(1.f - dot(cnd.xyz, tnd.xyz), dscale * abs(cnd.w - tnd.w)));
};

//
void calculateWeights(in ivec2 coord, inout float weights[9]) {
    for (int i=0;i<9;i++) { weights[i] = 0.f; };
    for (int cx=coord.x-1;cx<(coord.x+2);cx++) {
        for (int cy=coord.y-1;cy<(coord.y+2);cy++) {
            vec4 cnd = imageLoad(imageBuffers[2], ivec2(cx,cy));

            float sum = 0.f;
            float tmpWeights[9];
            for (int i=0;i<9;i++) { tmpWeights[i] = 0.f; };

            for (int i=-1;i<2;i++) {
                for (int j=-1;j<2;j++) {
                    ivec2 tap = ivec2(coord.x,coord.y)+(i,j);
                    if ((i - cx <= 1) && (j - cy <= 1)) {
                        float w = bilateral(cnd, imageLoad(imageBuffers[2], tap));
                        tmpWeights[(i+1)+(j+1)*3] = w;
                        sum += w;
                    };
                };
            };

            for (int i=0;i<9;i++) {
                weights[i] += tmpWeights[i] / sum;
            };
        };
    };
};

//
vec4 getAntiAliased(in ivec2 coord) {
    float weights[9];
    calculateWeights(coord, weights);

    vec3 result = vec3(0.f.xxx);
    for (int i=-1;i<2;i++) {
        for (int j=-1;j<2;j++) {
            result += weights[(i+1)+(j+1)*3] * (1.f/9.f) * imageLoad(imageBuffers[3], coord+ivec2(i,j)).xyz;
        };
    };
    return vec4(result, imageLoad(imageBuffers[3], coord).w);
};

// show ray tracing results
void main(){
    ivec2 coord = ivec2(vcoord * imageSize(imageBuffers[3]).xy);
    vec3 color = getAntiAliased(coord).xyz;//imageLoad(imageBuffers[3], coord).xyz;
    fragColor = vec4(color, 1.f);
};
