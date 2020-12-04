#ifndef FRAMEBUFFER_GLSL
#define FRAMEBUFFER_GLSL

#ifndef FRAMEBUFFER_MAP
#define FRAMEBUFFER_MAP 0
#endif

layout (binding = 0, set = FRAMEBUFFER_MAP, rgba32f) uniform image2D imageBuffers[];

#ifdef FRAGMENT
layout (location = 0) out vec4 fBarycentrics;
layout (location = 1) out vec4 fIndices;
#endif

#endif
