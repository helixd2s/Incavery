#ifndef GEOMETRY_REGISTRY_GLSL
#define GEOMETRY_REGISTRY_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"

#ifndef GEOMETRY_REGISTRY_MAP
#define GEOMETRY_REGISTRY_MAP 1
#endif

// what a f&ck?!
#define RawData uint64_t



layout(buffer_reference, scalar, buffer_reference_align = 1) buffer RawDataByte {
    uint8_t data[];
};

layout(buffer_reference, scalar, buffer_reference_align = 2) buffer RawDataShort {
    uint16_t data[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer RawDataUint {
    uint32_t data[];
};


//struct RawData {
//    uint32_t bufferId;
//    uint32_t offset;
//};

struct BindingInfo 
{
    uint32_t format;
    uint32_t stride;
    RawData ptr;
};


layout (binding = 0, set = GEOMETRY_REGISTRY_MAP, scalar) buffer AccelerationStructure { uint8_t data[]; } buffers[];
layout (binding = 1, set = GEOMETRY_REGISTRY_MAP, scalar) buffer BindingsBuffer { BindingInfo bindings[]; };

//#define bindingInfo bindings[bindingId]

uint8_t readUint8(in RawData ptr, in uint byteOffset) 
{
    return RawDataByte(ptr).data[byteOffset];
    //return buffers[nonuniformEXT(ptr.bufferId)].data[byteOffset+ptr.offset];
};

// 
uint16_t readUint16(in RawData ptr, in uint byteOffset) 
{
    if ((byteOffset&1)==0u) {
        return RawDataShort(ptr).data[byteOffset>>1u];
    } else {
        return pack16(u8vec2(readUint8(ptr, byteOffset+0u), readUint8(ptr, byteOffset+1u)));
    }
};

// 
uint32_t readUint32(in RawData ptr, in uint byteOffset) 
{
    if ((byteOffset&3)==0u) {
        return RawDataUint(ptr).data[byteOffset>>2u];
    } else {
        return pack32(u16vec2(readUint16(ptr, byteOffset+0u), readUint16(ptr, byteOffset+2u)));
    }
};



float readFloat(in RawData ptr, in uint byteOffset) 
{
    return uintBitsToFloat(readUint32(ptr, byteOffset));
};

vec4 readFloat4(in RawData ptr, in uint byteOffset) 
{
    return vec4(
        readFloat(ptr, byteOffset+0u),
        readFloat(ptr, byteOffset+4u),
        readFloat(ptr, byteOffset+8u),
        readFloat(ptr, byteOffset+12u)
    );
};

// read binding as float4
vec4 readBinding4(in BindingInfo bindingInfo, in uint index) 
{
    uint offset = bindingInfo.stride * index;
    return readFloat4(bindingInfo.ptr, offset);
};

mat3x4 readBindings3x4(in BindingInfo bindingInfo, in uvec3 indices) 
{
    return mat3x4(
        readBinding4(bindingInfo, indices.x),
        readBinding4(bindingInfo, indices.y),
        readBinding4(bindingInfo, indices.z)
    );
};


#endif
