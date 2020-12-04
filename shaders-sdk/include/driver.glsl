#ifndef DRIVER_GLSL
#define DRIVER_GLSL

#extension GL_EXT_scalar_block_layout           : require
#extension GL_EXT_shader_realtime_clock         : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier          : require
#extension GL_EXT_control_flow_attributes       : require
#extension GL_ARB_shader_clock                  : require

#extension GL_EXT_shader_explicit_arithmetic_types         : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8    : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16   : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32   : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64   : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_explicit_arithmetic_types_float32 : require
#extension GL_EXT_shader_explicit_arithmetic_types_float64 : require
#extension GL_EXT_shader_subgroup_extended_types_int8      : require
#extension GL_EXT_shader_subgroup_extended_types_int16     : require
#extension GL_EXT_shader_subgroup_extended_types_int64     : require
#extension GL_EXT_shader_subgroup_extended_types_float16   : require
#extension GL_EXT_shader_16bit_storage                     : require
#extension GL_EXT_shader_8bit_storage                      : require
#extension GL_KHR_shader_subgroup_basic                    : require
#extension GL_EXT_shader_atomic_float                      : require
#extension GL_KHR_shader_subgroup_arithmetic               : require
#extension GL_KHR_shader_subgroup_ballot                   : require

precision highp float;
precision highp int;


//
#define float2 vec2 
#define float3 vec3
#define float4 vec4
#define uint2 uvec2 
#define uint3 uvec3
#define uint4 uvec4
#define int2 ivec2 
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2 
#define bool3 bvec3
#define bool4 bvec4
#define float2x2 mat2x2 
#define float3x3 mat3x3
#define float4x4 mat4x4
#define float3x4 mat3x4
#define float4x3 mat4x3
#define half float16_t
#define half2 f16vec2
#define half3 f16vec3
#define half4 f16vec4
#define uint16_t2 u16vec2
#define uint16_t3 u16vec3
#define uint16_t4 u16vec4
#define ushort2 u16vec2
#define ushort3 u16vec3
#define ushort4 u16vec4
#define ubyte4 u8vec4
#define ubyte3 u8vec3
#define ubyte2 u8vec2
#endif

