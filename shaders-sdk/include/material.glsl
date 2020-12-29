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
layout (binding = 0, set = MATERIAL_MAP) uniform sampler2D textures[];
layout (binding = 1, set = MATERIAL_MAP, scalar) buffer MaterialBuffer { MaterialSource materials[]; };

// 
MaterialInfo handleMaterial(in uint materialId, inout AttributeInterpolated attributes) 
{
    MaterialInfo material;
    MaterialSource materialSource = materials[materialId];

    // base color 
    if (materialSource.baseColorTexture >= 0) {
        vec4 fetched = texture(textures[nonuniformEXT(materialSource.baseColorTexture)], attributes.texcoords.xy);
        material.baseColorFactor = fetched;
    } else {
        material.baseColorFactor = materialSource.baseColorFactor;
    };

    // metallic roughness 
    if (materialSource.metallicRoughnessTexture >= 0) {
        vec4 fetched = texture(textures[nonuniformEXT(materialSource.metallicRoughnessTexture)], attributes.texcoords.xy);
        material.metallicFactor = fetched.g;
        material.roughnessFactor = fetched.b;
    } else {
        material.metallicFactor = materialSource.metallicFactor;
        material.roughnessFactor = materialSource.roughnessFactor;
    };

    // transmission 
    if (materialSource.transmissionTexture >= 0) {
        vec4 fetched = texture(textures[nonuniformEXT(materialSource.transmissionTexture)], attributes.texcoords.xy);
        material.transmissionFactor = fetched.r;
    } else {
        material.transmissionFactor = materialSource.transmissionFactor;
    };

    return material;
};

#endif
