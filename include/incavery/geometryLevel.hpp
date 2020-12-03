#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./dataSet.hpp"

// 
namespace icv {

    // 
    struct Attributes 
    {
        uint32_t texcoords = 0u;
        uint32_t normals = 0u;
        uint32_t tangents = 0u;
        uint32_t colors = 0u;
    };

    // 
    struct VertexInfo 
    {
        uint32_t buffer = 0u;
        uint32_t offset = 0u;
        uint32_t stride = 16u;

        uint32_t max = 1u;
        uint32_t first = 0u;
    };

    // 
    struct IndexInfo 
    {
        uint32_t buffer = 0u;
        uint32_t type = 0u; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
        
    };

    //
    struct PrimitiveInfo 
    {
        uint32_t count = 3u;
        uint32_t reserved = 0u;
    };

    // 
    struct GeometryInfo 
    {
        glm::mat3x4 transform = glm::mat3x4(1.f);

        uint32_t 
            isOpaque: 1,
            useHalf : 1,
            hasTexcoords: 1, 
            hasNormals: 1,
            hasTangents: 1,
            hasColors: 1,
            hasTransform: 1;

        VertexInfo vertex = {};
        IndexInfo index = {};
        PrimitiveInfo count = {};

        Attributes attributes = {};
    };

    // 
    struct GeometryLevelInfo 
    {
        std::vector<GeometryInfo> geometries = {};
        
        uint32_t maxGeometryCount = 128u;
    };

    // 
    class GeometryLevel: public DeviceBased 
    {
        protected: 
        vkt::uni_ptr<GeometryRegistry> registry = {};

        // 
        GeometryLevelInfo info = {};
        BuildInfo buildInfo = {};

        // 
        vkt::uni_ptr<DataSet<GeometryInfo>> geometries = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        VkAccelerationStructureKHR acceleration = VK_NULL_HANDLE;
        vkt::Vector<uint8_t> accStorage = {};
        vkt::Vector<uint8_t> accScratch = {};

        //
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->geometries = std::make_shared<DataSet<GeometryInfo>>(device, DataSetInfo{
                .count = info->maxGeometryCount
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        public: 
        GeometryLevel() {};
        GeometryLevel(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) { this->constructor(device, info); };

        //
        virtual const vkt::Vector<GeometryInfo>& getBuffer() const {
            return geometries.getDeviceBuffer();
        };

        //
        virtual vkt::Vector<GeometryInfo>& getBuffer() {
            return geometries.getDeviceBuffer();
        };

        //
        virtual uint64_t getDeviceAddress() const {
            if (!acceleration) { this->makeAccelerationStructure(); };
            return device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = acceleration));
        };

        // 
        virtual void buildCommand(VkCommandBuffer commandBuffer) 
        {   
            {   // TODO: indirect condition
                geometries->copyFromVector(info.geometries);
                geometries->cmdCopyFromCpu(commandBuffer);
            };
            buildInfo.ranges.resize(info.geometries.size());
            for (uint32_t i=0;i<buildInfo.builds.size();i++) 
            {   // 
                buildInfo.ranges[i].firstVertex = info.geometries[i].vertex.first;
                buildInfo.ranges[i].primitiveCount = info.geometries[i].primitive.count;
                buildInfo.ranges[i].primitiveOffset = info.geometries[i].vertex.offset;
                buildInfo.ranges[i].transformOffset = sizeof(GeometryInfo) * i;
            };
            device->CmdBuildAccelerationStructuresKHR(commandBuffer, 1u, buildInfo.info, buildInfo.ranges.data());
        };

        // 
        virtual void makeAccelerationStructure() 
        {
            auto accelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            {   // 
                buildInfo.builds.resize(info.geometries.size());
                buildInfo.ranges.resize(info.geometries.size());
                for (uint32_t i=0;i<buildInfo.builds.size();i++) 
                {
                    // fill ranges
                    buildInfo.ranges[i].firstVertex = info.geometries[i].vertex.first;
                    buildInfo.ranges[i].primitiveCount = info.geometries[i].primitive.count;
                    buildInfo.ranges[i].primitiveOffset = info.geometries[i].vertex.offset;
                    buildInfo.ranges[i].transformOffset = sizeof(GeometryInfo) * i;

                    // fill build info
                    buildInfo.builds[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                    buildInfo.builds[i].geometry = vkh::VkAccelerationStructureGeometryTrianglesDataKHR
                    {
                        .vertexFormat = info.geometries[i].useHalf ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R32G32B32_SFLOAT,
                        .vertexData = ( registry->info.buffers[info.geometries[i].vertex.buffer] ).deviceAddress(),
                        .vertexStride = info.geometries[i].vertex.stride,
                        .maxVertex = info.geometries[i].vertex.max,
                        .indexType = getIndexType(info.geometries[i].index.type),
                        .indexData = ( registry->info.buffers[info.geometries[i].index.buffer] ).deviceAddress(),
                        .transformData = geometries.deviceAddress();
                    };

                    //
                    if (buildInfo.geometries[i].isOpaque) 
                    {
                        buildInfo.builds[i].flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
                    };
                };
                buildInfo.info.type = accelerationStructureType;
                buildInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
                buildInfo.info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfo.info.geometryCount = buildInfo.builds.size();
                buildInfo.info.pGeometries = buildInfo.builds.data();
            };

            vkh::VkAccelerationStructureBuildSizesInfoKHR sizes = {};
            {   // 
                std::vector<uint32_t> primitiveCount = {};
                for (uint32_t i=0;i<buildInfo.builds.size();i++) 
                {
                    primitiveCount.push_back(info.geometries[i].primitive.count);
                };

                // 
                device->dispatch->GetAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo.info, primitiveCounts.data(), &sizes);
            };

            {   // 
                this->accStorage = createBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizes.accelerationStructureSize);
                this->accScratch = createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizes.buildScratchSize);
            };

            {   // create acceleration structure
                vkh::VkAccelerationStructureCreateInfoKHR accelerationInfo = {};
                accelerationInfo.type = accelerationStructureType;
                accelerationInfo = this->accStorage;
                device->dispatch->CreateAccelerationStructureKHR(accelerationInfo, nullptr, &this->acceleration);
            };

            {   //
                buildInfo.info.dstAccelerationStructure = this->acceleration;
                buildInfo.info.scratchData = this->accScratch;
            };
        };

        // 
        virtual void flush(vkt::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([this,&](VkCommandBuffer commandBuffer) 
            {   // 
                this->buildCommand(commandBuffer);
            });
        };
    };

};
