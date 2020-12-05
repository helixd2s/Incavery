#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"

#ifndef MATERIAL_MAP
#define MATERIAL_MAP 3
#endif

// 
struct Material
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
Material handleMaterial(in uint materialId, inout AttributeInterpolated attributes) 
{
    Material material;
    
    return material;
};

#endif
