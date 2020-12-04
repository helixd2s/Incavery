#ifndef GEOMETRY_REGISTRY_GLSL
#define GEOMETRY_REGISTRY_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"

#ifndef GEOMETRY_REGISTRY_MAP
#define GEOMETRY_REGISTRY_MAP 1
#endif

struct BindingInfo 
{
    uint32_t format;
    uint32_t bufferId;
    uint32_t offset;
    uint32_t stride;
};

layout (binding = 0, set = GEOMETRY_REGISTRY_MAP) buffer AccelerationStructure { uint8_t data[]; } buffers[];
layout (binding = 1, set = GEOMETRY_REGISTRY_MAP) buffer BindingsBuffer { BindingInfo bindings[]; };

#define bindingInfo bindings[bindingId]

uint8_t readUint8(in uint bufferId, in uint byteOffset) 
{
    return buffers[bufferId].data[byteOffset+0u];
};

uint16_t readUint16(in uint bufferId, in uint byteOffset) 
{
    return pack16(u8vec2(
        readUint8(bufferId, byteOffset+0u),
        readUint8(bufferId, byteOffset+1u)
    ));
};

uint32_t readUint32(in uint bufferId, in uint byteOffset) 
{
    return pack32(u16vec2(
        readUint16(bufferId, byteOffset+0u),
        readUint16(bufferId, byteOffset+2u)
    ));
};



float readFloat(in uint bufferId, in uint byteOffset) 
{
    return uintBitsToFloat(readUint32(bufferId, byteOffset));
};

vec4 readFloat4(in uint bufferId, in uint byteOffset) 
{
    return vec4(
        readFloat(bufferId, byteOffset+0u),
        readFloat(bufferId, byteOffset+4u),
        readFloat(bufferId, byteOffset+8u),
        readFloat(bufferId, byteOffset+12u)
    );
};

// read binding as float4
vec4 readBinding(in uint bindingId, in uint index) 
{
    //BindingInfo bindingInfo = bindings[bindingId];
    uint offset = bindingInfo.stride * index + bindingInfo.offset;
    return readFloat4(bindingInfo.bufferId, offset);
};

mat3x4 readAttribute(in uint bindingId, in uvec3 indices) 
{
    return mat3x4(
        readBinding(bindingId, indices.x),
        readBinding(bindingId, indices.y),
        readBinding(bindingId, indices.z)
    );
};


#endif
