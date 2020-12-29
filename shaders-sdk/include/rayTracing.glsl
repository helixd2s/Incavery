#ifndef RAY_TRACING_GLSL
#define RAY_TRACING_GLSL

#include "./driver.glsl"
#include "./constants.glsl"
#include "./common.glsl"
#include "./geometryRegistry.glsl"
#include "./instanceLevel.glsl"
#include "./material.glsl"

layout (binding = 1, set = INSTANCE_LEVEL_MAP) uniform accelerationStructureEXT acceleration;

IntersectionInfo traceRays(in RayData rays, in float maxT) {
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, acceleration, gl_RayFlagsNoneEXT, 0xff, rays.origin.xyz, 0.001f, rays.direction.xyz, maxT);

    while(rayQueryProceedEXT(rayQuery)) {
        bool isOpaque = true;

        {   // compute intersection opacity
            uint instanceId = rayQueryGetIntersectionInstanceIdEXT(rayQuery, false);
            uint geometryId = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
            uint primitiveId = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            GeometryInfo geometryInfo = readGeometryInfo(instanceId, geometryId);
            uvec3 indices = readIndices(geometryInfo.index, primitiveId);
            AttributeMap attributeMap = readAttributes(geometryInfo.attributes, indices);
            vec2 attribs = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
            AttributeInterpolated attributes = interpolateAttributes(attributeMap, vec3(1.f - attribs.x - attribs.y, attribs));
            MaterialInfo material = handleMaterial(geometryInfo.primitive.materials, attributes);

            if (material.baseColorFactor.a < 0.0001f) {
                isOpaque = false;
            };

        };

        if (isOpaque) {
            rayQueryConfirmIntersectionEXT(rayQuery);
        };
    };

    IntersectionInfo result;

    {
        result.barycentric = vec3(0.f.xxx);
        result.hitT = maxT;
        result.instanceId = 0u;
        result.geometryId = 0u;
        result.primitiveId = 0u;
    };

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
        vec2 attribs = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        result.barycentric = vec3(1.f - attribs.x - attribs.y, attribs);
        result.hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
        result.instanceId = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
        result.geometryId = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
        result.primitiveId = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    };

    return result;
};

#endif
