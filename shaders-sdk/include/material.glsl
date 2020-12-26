#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"

#ifndef MATERIAL_MAP
#define MATERIAL_MAP 3
#endif

// 
struct MaterialInfo
{
    vec4 baseColorFactor; // KHR
    float metallicFactor; // KHR
    float roughnessFactor; // KHR
    float transmissionFactor; // KHR
    float adobeIor; // Adobe

    vec4 normalFactor; // normal map value (not applied)
    vec4 modifiedNormal; // normal mapped normal
};

//
struct MaterialSource
{
    // factors
    vec4 baseColorFactor;
    float metallicFactor; // KHR
    float roughnessFactor; // KHR
    float transmissionFactor; // KHR
    float adobeIor; // Adobe

    // textures
    int baseColorTexture;
    int metallicRoughnessTexture;
    int transmissionTexture;
    int normalTexture;
};

// 
MaterialSource handleMaterial(in uint materialId, inout AttributeInterpolated attributes) 
{
    MaterialSource material;
    
    return material;
};

// 
layout (binding = 0, set = MATERIAL_MAP) uniform sampler2D textures[];
layout (binding = 1, set = MATERIAL_MAP, scalar) buffer MaterialBuffer { MaterialSource materials[]; };

#endif
