#ifndef EXTERNAL_GLSL
#define EXTERNAL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"

//
layout(binding = 0, set = 4) uniform Constants {
    mat4x4 perspective;
    mat4x4 perspectiveInverse;
    mat3x4 lookAt;
    mat3x4 lookAtInverse;
} constants;

//
layout(binding = 1, set = 4, rgba32f) uniform image2D fOutput[];

// 
vec4 GetTextureLinear(in vec2 txc) {
    const uint I = 0u;
    const ivec2 size = imageSize(fOutput[I]);
    const vec2 txy = txc*vec2(size)-0.5f;
    const vec2 ttf = fract(txy);
    const mat4x4 txl = mat4x4(
        imageLoad(fOutput[I], ivec2(txy)+ivec2(0,0)),
        imageLoad(fOutput[I], ivec2(txy)+ivec2(1,0)),
        imageLoad(fOutput[I], ivec2(txy)+ivec2(0,1)),
        imageLoad(fOutput[I], ivec2(txy)+ivec2(1,1))
    );
    const vec2 px = vec2(1.f-ttf.x,ttf.x), py = vec2(1.f-ttf.y,ttf.y);
    const mat2x2 i2 = outerProduct(px,py);
    return txl * vec4(i2[0],i2[1]); // interpolate
}

#endif
