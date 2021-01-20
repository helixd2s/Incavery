#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./dataSet.hpp"

// 
namespace icv {

#pragma pack(push, 8)
    // 
    struct Attributes 
    {
        int32_t texcoords = -1;
        int32_t normals = -1;
        int32_t tangents = -1;
        int32_t colors = -1;
    };

    // 
    struct IndexInfo 
    {
        int32_t first = 0;
        uint32_t max = 1u;
        uint32_t reserved = 0u;
        uint32_t type = 0u; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
        RawData ptr = {};
    };

    //
    struct PrimitiveInfo 
    {
        uint32_t offset = 0u;
        uint32_t count = 1u;
        uint32_t materials = 0u; // binding
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
        uint32_t type = 0u; // zero is triangle, other are custom and defined by custom shaders (NOT drawn by rasterization due difficults)

        uint32_t aabbs = 0u; // required for custom geometry
        uint32_t vertex = 0u;

        IndexInfo index = {};
        PrimitiveInfo primitive = {};

        Attributes attributes = {};
    };
#pragma pack(pop)


    // 
    struct GeometryLevelInfo 
    {
        vkh::uni_ptr<GeometryRegistry> registry = {};
        std::vector<GeometryInfo> geometries = {};

        uint32_t maxGeometryCount = 128u;
    };

    // 
    class GeometryLevel: public DeviceBased 
    {
        protected: 

        // 
        GeometryLevelInfo info = {};
        BuildInfo buildInfo = {};

        // 
        vkh::uni_ptr<DataSet<GeometryInfo>> geometries = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        VkAccelerationStructureKHR acceleration = VK_NULL_HANDLE;
        vkf::VectorBase accStorage = {};
        vkf::VectorBase accScratch = {};

        //
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->geometries = std::make_shared<DataSet<GeometryInfo>>(device, DataSetInfo{
                .count = info->maxGeometryCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        public: 
        GeometryLevel() {};
        GeometryLevel(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) { this->constructor(device, info); };

        //
        virtual const VkDescriptorSet& getDescriptorSet() const {
            return set;
        };

        //
        virtual VkDescriptorSet& getDescriptorSet() {
            return set;
        };

        //
        virtual const GeometryLevelInfo& getInfo() const {
            return info;
        };

        //
        virtual GeometryLevelInfo& getInfo() {
            return info;
        };

        //
        virtual const vkf::Vector<GeometryInfo>& getBuffer() const {
            return geometries->getDeviceBuffer();
        };

        //
        virtual vkf::Vector<GeometryInfo>& getBuffer() {
            return geometries->getDeviceBuffer();
        };

        //
        virtual VkDeviceAddress getDeviceAddress() {
            if (!acceleration) { this->makeAccelerationStructure(); };
            return device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = acceleration));
        };

        //
        //virtual VkDeviceAddress getDeviceAddress() const {
        //    vkh::VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {};
        //    return device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = acceleration));
        //};

        // 
        virtual void buildCommand(VkCommandBuffer commandBuffer) 
        {   
            if (!acceleration) { this->makeAccelerationStructure(); };
            {   // TODO: indirect condition
                geometries->copyFromVector(info.geometries);
                geometries->cmdCopyFromCpu(commandBuffer);
            };
            buildInfo.ranges.resize(info.geometries.size());
            for (uint32_t i=0;i<buildInfo.builds.size();i++) 
            {   // 
                buildInfo.ranges[i].firstVertex = info.geometries[i].index.first;
                buildInfo.ranges[i].primitiveCount = info.geometries[i].primitive.count;
                buildInfo.ranges[i].primitiveOffset = info.geometries[i].primitive.offset;
                buildInfo.ranges[i].transformOffset = sizeof(GeometryInfo) * i;
            };

            const auto ptr = &buildInfo.ranges[0u];
            device->dispatch->CmdBuildAccelerationStructuresKHR(commandBuffer, 1u, buildInfo.info, &ptr);
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
                    buildInfo.ranges[i].firstVertex = info.geometries[i].index.first;
                    buildInfo.ranges[i].primitiveCount = info.geometries[i].primitive.count;
                    buildInfo.ranges[i].primitiveOffset = info.geometries[i].primitive.offset;
                    buildInfo.ranges[i].transformOffset = sizeof(GeometryInfo) * i;

                    // fill build info
                    buildInfo.builds[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                    if (info.geometries[i].type == 0u) {
                        auto& vertex = info.registry->getInfo().bindings[info.geometries[i].vertex];
                        buildInfo.builds[i].geometry = vkh::VkAccelerationStructureGeometryTrianglesDataKHR
                        {
                            .vertexFormat = info.geometries[i].useHalf ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R32G32B32_SFLOAT,
                            .vertexData = vertex.ptr.data /*bufferDeviceAddress(info.registry->getInfo().buffers[vertex.ptr.bufferId]) + vertex.ptr.offset*/,
                            .vertexStride = vertex.stride,
                            .maxVertex = info.geometries[i].index.max,
                            .indexType = getIndexType(info.geometries[i].index.type),
                            .indexData = info.geometries[i].index.ptr.data /*bufferDeviceAddress(info.registry->getInfo().buffers[info.geometries[i].index.ptr.bufferId]) + info.geometries[i].index.ptr.offset*/,
                            .transformData = geometries->getDeviceBuffer().deviceAddress()
                        };
                    };

                    //
                    if (info.geometries[i].isOpaque) 
                    {
                        buildInfo.builds[i].flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
                    };
                };
                buildInfo.info.type = accelerationStructureType;
                buildInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
                buildInfo.info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfo.info.geometryCount = buildInfo.builds.size();
                buildInfo.info.pGeometries = &buildInfo.builds[0u];
            };

            vkh::VkAccelerationStructureBuildSizesInfoKHR sizes = {};
            {   // 
                std::vector<uint32_t> primitiveCount = {};
                for (uint32_t i=0;i<buildInfo.builds.size();i++) 
                {
                    primitiveCount.push_back(info.geometries[i].primitive.count);
                };

                // 
                device->dispatch->GetAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo.info, primitiveCount.data(), &sizes);
            };

            {   // 
                this->accStorage = createBuffer(BufferCreateInfo{.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, .size = sizes.accelerationStructureSize});
                this->accScratch = createBuffer(BufferCreateInfo{.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, .size = sizes.buildScratchSize});
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
        uintptr_t pushGeometry(vkh::uni_arg<GeometryInfo> geometryInfo) {
            uintptr_t last = info.geometries.size();
            info.geometries.push_back(geometryInfo);
            return last;
        };

        //
        void setGeometry(uintptr_t index, vkh::uni_arg<GeometryInfo> geometryInfo) {
            if (info.geometries.size() <= index) { info.geometries.resize(index+1u); };
            info.geometries[index] = geometryInfo;
        };

        // 
        virtual void flush(vkh::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer) 
            {   // 
                this->buildCommand(commandBuffer);
            });
        };
    };

};
